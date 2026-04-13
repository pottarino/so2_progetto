    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdbool.h>

#include "errors.h"

    typedef enum {
        TOKEN_UNKNOWN,
        TOKEN_KEYWORD,    // if, while, return...
        TOKEN_TYPE,       // int, char, double, custom_t...
        TOKEN_MODIFIER,   // const, static, volatile, unsigned...
    } TokenType;


    typedef struct {
        char* buffer;
        StatusCode statusCode;
    } FileRead;

    typedef struct {
        char** items;
        int count;
    } Dictionary;

    Dictionary dict_keywords = {NULL, 0};
    Dictionary dict_types = {NULL, 0};
    Dictionary dict_modifiers = {NULL, 0};

    //gestione dizionari
    void add_to_dict(Dictionary* dict, const char* word) {
        for(int i = 0; i < dict->count; i++) {
            if (strcmp(dict->items[i], word) == 0) return;
        }
        char** temp = realloc(dict->items, (dict->count + 1) * sizeof(char*));
        if (temp) {
            dict->items = temp;
            dict->items[dict->count] = strdup(word);
            dict->count++;
        }
    }

    //inizializzazione
    void init_syntax() {
        //controlli
        const char* k[] = {"if", "else", "for", "while", "do", "switch", "case", "break", "continue", "return", "sizeof", "typedef"};
        for(int i=0; i<12; i++) add_to_dict(&dict_keywords, k[i]);

        // tipi
        const char* t[] = {"int", "char", "float", "double", "void", "long", "short", "bool"};
        for(int i=0; i<8; i++) add_to_dict(&dict_types, t[i]);

        // modificatori
        const char* m[] = {"const", "static", "volatile", "unsigned", "signed", "extern", "register"};
        for(int i=0; i<7; i++) add_to_dict(&dict_modifiers, m[i]);
    }

    //ritorna il tipo di token
    TokenType get_token_type(const char* word) {
        if (!word) return TOKEN_UNKNOWN;

        for(int i=0; i<dict_modifiers.count; i++)
            if(strcmp(word, dict_modifiers.items[i]) == 0) return TOKEN_MODIFIER;

        for(int i=0; i<dict_types.count; i++)
            if(strcmp(word, dict_types.items[i]) == 0) return TOKEN_TYPE;

        for(int i=0; i<dict_keywords.count; i++)
            if(strcmp(word, dict_keywords.items[i]) == 0) return TOKEN_KEYWORD;

        return TOKEN_UNKNOWN;
    }

    int is_known_type(const char* word) {
        return get_token_type(word) == TOKEN_TYPE;
    }

    int is_qualifier(const char* word) {
        return get_token_type(word) == TOKEN_MODIFIER;
    }

    char** split(const char* src, const char *splitter) {
        if (!src || !splitter) return NULL;
        char* copy = strdup(src);
        char** result = NULL;
        int count = 0;
        char* token = strtok(copy, splitter);

        while (token) {
            char** temp = realloc(result, (count + 2) * sizeof(char*));
            if (!temp) break;
            result = temp;
            result[count++] = strdup(token);
            result[count] = NULL;
            token = strtok(NULL, splitter);
        }
        free(copy);
        return result;
    }

    // libera la memoriadello split
    void free_split(char** words) {
        if (!words) return;
        for (int i = 0; words[i] != NULL; i++) free(words[i]);
        free(words);
    }

    int is_variable_declaration(const char* line) {
        char** words = split(line, " ");
        if (!words || !words[0]) {
            free_split(words);
            return 0;
        }
        int i = 0;
        bool type_found = false;

        //salta i modificatori (possono essercene più di uno: static const...)
        while (words[i] && get_token_type(words[i]) == TOKEN_MODIFIER) {
            i++;
        }

        //verifica se c'è un tipo
        if (words[i] && get_token_type(words[i]) == TOKEN_TYPE) {
            type_found = true;
            i++;
        }

        if (!type_found) {
            free_split(words);
            return 0;
        }

        // analisi del resto della riga
        //se dopo il tipo troviamo subito una '(', è una funzione, non una variabile
        while (words[i]) {
            //se contiene una parentesi e non c'è un assegnamento '=', è una funzione
            if (strchr(words[i], '(') && !strchr(words[i], '=')) {
                free_split(words);
                return 0;
            }

            //sse troviamo un carattere alfabetico o un asterisco, è potenzialmente un nome
            if (strpbrk(words[i], "*_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")) {
                free_split(words);
                return 1;
            }
            i++;
        }
        free_split(words);
        return 0;
    }

#include <stdio.h>
#include <stdlib.h>

/**
 * legge il contenuto di un file e lo alloca in un buffer.
 * gestisce gli errori tramite StatusCode e pulizia con goto.
 */
FileRead filereader(char* const filedest) {
        char* buffer = NULL;
        StatusCode codiceStato = NO_ERROR;
        FILE *fp = NULL;

        fp = fopen(filedest, "r");
        if (!fp) {
            codiceStato = FILE_OPEN_ERROR;
            goto exitHandler;
        }

        // calcolo dimensione file
        if (fseek(fp, 0, SEEK_END) != 0) {
            codiceStato = FILE_READ_ERROR;
            goto exitHandler;
        }

        long size = ftell(fp);
        if (size <= 0) {
            codiceStato = FILE_READ_ERROR;
            goto exitHandler;
        }

        rewind(fp);

        // allocazione memoria
        buffer = malloc(size + 1);
        if (!buffer) {
            codiceStato = FILE_READ_ERROR; // o MALLOC_ERROR se definito
            goto exitHandler;
        }

        // lettura effettiva
        size_t read_size = fread(buffer, 1, size, fp);
        if (read_size < (size_t)size) {
            codiceStato = FILE_READ_ERROR;
            free(buffer);
            buffer = NULL;
            goto exitHandler;
        }

        buffer[size] = '\0';

        exitHandler:
            if (fp) {
                if (fclose(fp) != 0 && codiceStato == NO_ERROR) {
                    codiceStato = FILE_CLOSE_ERROR;
                }
            }

        FileRead result = {buffer, codiceStato};
        return result;
    }



    int is_keyword(const char* word) {
        return get_token_type(word) != TOKEN_UNKNOWN;
    }