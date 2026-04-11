//
// Created by potta on 02/04/2026.
//

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