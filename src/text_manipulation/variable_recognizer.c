//
// Created by potta on 02/04/2026.
//

#include "variable_recognizer.h"

#include "utility.h"
#include "parsers.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int is_name_valid(const char *name) {
    if (!name || name[0] == '\0') return 0;

    //se è un assegnamento puro lo saltiamo e diciamo ok
    if (name[0] == '=') return 1;

    //un nome non può mai iniziare con un numero
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
        char** words = split(codeline.lines[i], " ");
        if (!words || !words[0]) {
            free_split(words);
            continue;
        }

        int j = 0;
        //mangia tipi, modificatori e asterischi sparsi
        while (words[j] != NULL && (is_qualifier(words[j]) || is_known_type(words[j]) || strcmp(words[j], "*") == 0)) {
            j++;
        }

        //se abbiamo trovato una testa valida (es. "int")
        if (j > 0 && words[j] != NULL) {
            int at_least_one_var = 0;
            int all_tokens_valid = 1;

            while (words[j] != NULL) {
                //se becca una tonda prima di un uguale è una funzione
                if (strchr(words[j], '(') != NULL && strchr(words[j], '=') == NULL) {
                    all_tokens_valid = 0;
                    break;
                }

                //se il token contiene un uguale (anche "x=10" o "val=")
                if (strchr(words[j], '=') != NULL) {
                    at_least_one_var = 1;
                    //una volta visto l'uguale, saltiamo tutto fino a ',' o ';'
                    //perchè a destra può esserci qualsiasi cosa (es: (x+y)*2 )
                    while (words[j] != NULL && strchr(words[j], ',') == NULL && strchr(words[j], ';') == NULL) {
                        j++;
                    }
                    if (words[j] == NULL) break;

                    //se il token su cui siamo fermi ha una virgola, continuiamo il check
                    if (strchr(words[j], ';') != NULL) break;
                } else {
                    //validazione nome standard per i pezzi senza uguale
                    if (!is_name_valid(words[j])) {
                        all_tokens_valid = 0;
                        break;
                    } else {
                        at_least_one_var = 1;
                    }
                }

                if (words[j] != NULL && strchr(words[j], ';') != NULL) break;
                j++;
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


Variable* parse_variable_declaration(CodeLine codeline, int* total_found) {
    *total_found = 0;
    Variable* var_list = NULL;

    char** words = split(codeline.lines[0], " ");
    if (!words) return NULL;

    int j = 0;
    char temp_type[512] = "";
    int type_valid = 1;

    // 1. isola il tipo per tutta la riga
    while (words[j] != NULL && (is_qualifier(words[j]) || is_known_type(words[j]) || strcmp(words[j], "*") == 0)) {
        if (is_keyword(words[j]) && !is_known_type(words[j]) && !is_qualifier(words[j])) {
            type_valid = 0;
            break;
        }
        strcat(temp_type, words[j]);
        strcat(temp_type, " ");
        j++;
    }

    if (strlen(temp_type) > 0) {
        temp_type[strlen(temp_type) - 1] = '\0';
    } else {
        type_valid = 0;
        strcpy(temp_type, "unknown");
    }

    // 2. crea una struct per ogni nome trovato
    while (words[j] != NULL) {
        if (strchr(words[j], '(') != NULL && strchr(words[j], '=') == NULL) break;

        char clean_name[256];
        int k = 0, m = 0;
        // pulizia al volo
        while (words[j][k] != '\0' && words[j][k] != '=' && m < 255) {
            if (words[j][k] != '*' && words[j][k] != ',' && words[j][k] != ';' &&
                words[j][k] != '[' && words[j][k] != ']') {
                clean_name[m++] = words[j][k];
            }
            k++;
        }
        clean_name[m] = '\0';

        if (strlen(clean_name) > 0) {
            var_list = realloc(var_list, sizeof(Variable) * (*total_found + 1));
            Variable* v = &var_list[*total_found];

            // setup dati
            // Mentre scorri i token (words[j])
            if (strcmp(words[j], "=") == 0) {
                // Se è un uguale isolato, saltalo e non creare una Variable!
                j++;
                continue;
            }

            // Se il token pulito è vuoto o è solo punteggiatura, saltalo
            if (strlen(clean_name) == 0 || strcmp(clean_name, "*") == 0) {
                j++;
                continue;
            }
            v->type = strdup(temp_type);
            v->name = strdup(clean_name);
            v-> line = codeline;
            v->deletedBit = 0;
            v->errors = NULL;
            v->errors_count = 0;

            // check errori (type o name)
            if (!type_valid) {
                v->errors = malloc(sizeof(VariableError));
                *(v->errors) = VARIABLE_TYPE_ERROR;
                v->errors_count++;
            }
            if (is_keyword(clean_name) || !is_name_valid(clean_name)) {
                v->errors = malloc(sizeof(VariableError));
                *(v->errors) = VARIABLE_NAME_ERROR;
                v->errors_count++;
            }

            (*total_found)++;
        }

        // salta l'inizializzazione se c'è
        if (strchr(words[j], '=') != NULL) {
            while (words[j] != NULL && strchr(words[j], ',') == NULL && strchr(words[j], ';') == NULL) {
                j++;
            }
            if (words[j] == NULL) break;
        }

        if (strchr(words[j], ';') != NULL) break;
        j++;
    }

    free_split(words);
    return var_list;
}

// libera l'array di variabili estratte
void free_variables(Variable* v, int count) {
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

char* getStructType(CodeLine cl) {
    // Supponendo che cl.lines[0] sia "typedef struct Point Point;" o simile
    // Molto ignorante: cerchiamo l'ultima parola prima del punto e virgola
    if (cl.count == 0 || cl.lines[0] == NULL) return "unknown";
    char** words = split(cl.lines[0], " ");
    int i = 0;
    while (words[i] != NULL) i++;

    // Ritorna l'ultima parola (il nome del tipo definito)
    char* result = strdup(words[i-1]);
    // Pulisci eventuale ';' alla fine
    char* semi = strchr(result, ';');
    if (semi) *semi = '\0';

    free_split(words);
    return result;
}