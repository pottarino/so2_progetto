#include "utility.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errors.h"
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

int startswith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return 0;
    }
    return 1;
}

char** split(const char* src, const char *splitter) {
    if (!src || !splitter) {
        return NULL;
    }

    char* copy = strdup(src);
    if (!copy) return NULL; // Controllo sicurezza allocazione

    char** result = NULL;
    int count = 0;
    char* token = strtok(copy, splitter);
    while (token) {
        // Usiamo una dimensione temporanea per sicurezza
        char** temp = realloc(result, (count + 1) * sizeof(char*));
        if (!temp) {
            // Se fallisce, dobbiamo liberare quello che abbiamo fatto finora
            for(int i=0; i<count; i++) free(result[i]);
            free(result);
            free(copy);
            return NULL;
        }
        result = temp;
        result[count] = strdup(token); // Copia reale
        count++;
        token = strtok(NULL, splitter);
    }

    // Aggiungiamo il terminatore NULL alla fine (necessario per il ciclo while(words[j]))
    char** final_temp = realloc(result, (count + 1) * sizeof(char*));
    if (final_temp) {
        result = final_temp;
        result[count] = NULL;
    }

    free(copy); // Adesso è sicuro liberare la copia di lavoro
    return result;
}

// libera la memoriadello split
void free_split(char** words) {
if (!words) return;
for (int i = 0; words[i] != NULL; i++) free(words[i]);
free(words);
}

/**
* legge il contenuto di un file e lo alloca in un buffer.
* gestisce gli errori tramite StatusCode e pulizia con goto.
*/
FileRead file_reader(char* const filedest) {
char* buffer = NULL;
StatusCode codiceStato = NO_ERROR;
FILE *fp = NULL;

fp = fopen(filedest, "rb");
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

int allocate_more(void **pointer, int *current_capacity, size_t element_size) {
    if (!pointer || !current_capacity || element_size == 0) return 0;

    // Se la capacità è 0, iniziamo con un valore base (es. 4)
    int new_capacity = (*current_capacity <= 0) ? 4 : (*current_capacity * 2);

    void *temp = realloc(*pointer, new_capacity * element_size);

    if (!temp) {
        // Fallimento realloc: il vecchio puntatore è ancora valido, non facciamo nulla
        return 0;
    }

    *pointer = temp;
    *current_capacity = new_capacity;
    return 1;
}

int free_unused(void **pointer, int new_size) {
if (!pointer || !(*pointer)) return 0;

// se la nuova size è 0, liberiamo tutto
if (new_size <= 0) {
free(*pointer);
*pointer = NULL;
return 1;
}

void *temp = realloc(*pointer, new_size);

if (!temp) {
// realloc verso il basso raramente fallisce, ma check obbligatorio
return 0;
}

*pointer = temp;
return 1;}

void clean_newline(char* str) {
    if (!str) return;
    size_t len = strlen(str);
    // Cicla a ritroso e trasforma \r e \n in terminatori di stringa
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}