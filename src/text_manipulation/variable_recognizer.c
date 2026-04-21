#include "variable_recognizer.h"
#include "utility.h"
#include "parsers.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int is_name_valid(const char *name) {
    if (!name || name[0] == '\0') return 0;

    if (name[0] == '=') return 1;

    if (isdigit((unsigned char)name[0])) return 0;

    char clean[256];
    int j = 0;
    //puliamo il token da punteggiatura varia attaccata dallo split
    for (int i = 0; name[i] != '\0' && j < 255; i++) {
        if (name[i] != '*' && name[i] != ',' && name[i] != ';') {
            clean[j++] = name[i];
        }
    }
    clean[j] = '\0';

    //fondamentale: se dopo la pulizia è una keyword tipo 'if' o 'int', non è un nome
    if (is_keyword(clean)) return 0;

    //check alfanumerico classico
    if (!isalpha((unsigned char)clean[0]) && clean[0] != '_') return 0;
    for (int i = 1; clean[i] != '\0'; i++) {
        if (!isalnum((unsigned char)clean[i]) && clean[i] != '_') return 0;
    }

    return 1;
}

/**
 * determina se una CodeLine contiene una dichiarazione di variabile.
 * implementa un'analisi sequenziale: Modificatori -> Tipo -> Identificatori.
 */
int is_variable(CodeLine codeline) {

    for (int i = 0; i < codeline.count; i++) {
        fflush(stdout);
        char **words = split(codeline.lines[i], " ");
        fflush(stdout);
        if (!words || !words[0]) {
            free_split(words);
            continue;
        }

        if (!is_known_type(words[0]) && !is_qualifier(words[0]) && strcmp(words[0], "*") != 0) {
            free_split(words);
            continue;
        }

        int j = 0;
        // 1. Mangia i tipi
        while (words[j] != NULL && (is_qualifier(words[j]) || is_known_type(words[j]) || strcmp(words[j], "*") == 0)) {
            j++;
        }

        // 2. Analizza i nomi delle variabili
        if (words[j] != NULL) {
            int at_least_one_var = 0;
            int all_tokens_valid = 1;

            while (words[j] != NULL) {
                // Controllo parentesi (funzione)
                if (strchr(words[j], '(') != NULL && strchr(words[j], '=') == NULL) {
                    all_tokens_valid = 0;
                    break;
                }

                if (strchr(words[j], '=') != NULL) {
                    at_least_one_var = 1;
                    // SALTO SICURO: incrementa j e controlla SEMPRE che non sia NULL
                    while (words[j] != NULL) {
                        if (strchr(words[j], ',') || strchr(words[j], ';')) break;
                        j++;
                    }
                    // Se siamo arrivati in fondo senza virgole o punti e virgola
                    if (words[j] == NULL) break;
                }
                else {
                    if (!is_name_valid(words[j])) {
                        all_tokens_valid = 0;
                        break;
                    }
                    at_least_one_var = 1;
                }

                // Controllo finale sul token attuale prima di incrementare
                if (strchr(words[j], ';') != NULL) break;

                j++; // Incremento unico e controllato
            }

            if (all_tokens_valid && at_least_one_var) {
                free_split(words);
                return 1;
            }
        }
        free_split(words);
    }
    return 0;
}

void extract_pure_identifier(const char *src, char *dest) {
    int i = 0, j = 0;

    // Salta eventuali simboli iniziali che non fanno parte del nome (es: *ptr o &var)
    while (src[i] != '\0' && !isalnum((unsigned char)src[i]) && src[i] != '_') {
        i++;
    }

    // Copia finché incontra caratteri validi per un identificatore C (lettere, numeri, underscore)
    while (src[i] != '\0' && (isalnum((unsigned char)src[i]) || src[i] == '_')) {
        if (j >= 255) break;
        dest[j++] = src[i++];
    }

    // Chiude la stringa
    dest[j] = '\0';
}

Variable *parse_variable_declaration(CodeLine codeline, int *total_found) {
    *total_found = 0;
    if (codeline.count == 0 || codeline.lines[0] == NULL) return NULL;

    Variable *var_list = NULL;
    char **words = split(codeline.lines[0], " ");
    if (!words || !words[0]) return NULL;

    int j = 0;
    char temp_type[512] = "";

    // 1. Isola il tipo (uguale per tutti i nomi sulla riga)
    while (words[j] != NULL && (is_qualifier(words[j]) || is_known_type(words[j]) || strcmp(words[j], "*") == 0)) {
        strncat(temp_type, words[j], sizeof(temp_type) - strlen(temp_type) - 2);
        strcat(temp_type, " ");
        j++;
    }
    if (strlen(temp_type) > 0) temp_type[strlen(temp_type) - 1] = '\0';

    // 2. Ciclo principale: salva ogni nome trovato come nuova Variable
    while (words[j] != NULL) {
        // Se incontra una funzione, interrompe
        if (strchr(words[j], '(') != NULL && strchr(words[j], '=') == NULL) break;

        char clean_name[256];
        extract_pure_identifier(words[j], clean_name);

        // Se abbiamo un nome valido, allochiamo e salviamo
        if (clean_name[0] != '\0' && !is_known_type(clean_name) && !is_qualifier(clean_name)) {
            Variable *temp = realloc(var_list, sizeof(Variable) * (*total_found + 1));
            if (!temp) { free_split(words); return var_list; }
            var_list = temp;
            Variable *v = &var_list[(*total_found)++];

            v->type = strdup(temp_type);
            v->name = strdup(clean_name);
            v->line = codeline;
            v->deletedBit = 0;
            v->used = 0;
            v->errors_count = 0;
            v->errors = NULL;
        }

        // Se c'è un uguale, salta tutto fino alla prossima virgola o fine riga
        if (strchr(words[j], '=') != NULL) {
            while (words[j] != NULL && strchr(words[j], ',') == NULL && strchr(words[j], ';') == NULL) {
                j++;
            }
        }

        // Se siamo su un token con la virgola, avanziamo per il prossimo nome
        if (words[j] != NULL && strchr(words[j], ',') != NULL) {
            j++;
            continue;
        }

        // Se c'è il punto e virgola, la riga è finita
        if (words[j] == NULL || strchr(words[j], ';') != NULL) break;

        j++;
    }

    free_split(words);
    return var_list;
}

// libera l'array di variabili estratte
void free_variable_array(Variable *v , int count){
    if (!v) return;
    for (int i = 0; i < count; i++) {
        free(v[i].type);
        v[i].type = NULL;
        free(v[i].name);
        v[i].name = NULL;
        // libera l'array errori se c'è (mallocata nel parser)
        if (v[i].errors) free(v[i].errors);
    }
    free(v);
}

char *getStructType(CodeLine cl) {
    // Supponendo che cl.lines[0] sia "typedef struct Point Point;" o simile
    // Molto ignorante: cerchiamo l'ultima parola prima del punto e virgola
    if (cl.count == 0 || cl.lines[0] == NULL) return strdup("unknown");

    char **words = split(cl.lines[0], " ");
    if (!words) return strdup("unknown");

    int i = 0;
    while (words[i] != NULL) i++;
    if (i == 0) { free_split(words); return strdup("unknown"); }

    // Ritorna l'ultima parola (il nome del tipo definito)
    char *result = strdup(words[i - 1]);

    // Pulisci eventuale ';' alla fine
    if (result) {
        char *semi = strchr(result, ';');
        if (semi) *semi = '\0';
    }

    free_split(words);
    return result;
}