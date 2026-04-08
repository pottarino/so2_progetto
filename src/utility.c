#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errors.h>

char* standard_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
};

char* *custom_keywords = NULL;

int reserved_keywords_count = 32;

int custom_keywords_count = 0;

int is_keyword(const char* name) {
    for (int i = 0; i < 32; i++) {
        if (strcmp(name, standard_keywords[i]) == 0) return 1; // se è una standard
    }

    for (int i = 0; i < custom_keywords_count; i++) {
        if (strcmp(name, custom_keywords[i]) == 0) return 1; // o una aggiunta dall'utente
    }

    return 0;
}


void add_reserved_keyword(const char *new_keyword) {
    if (is_keyword(new_keyword)) return;

    char** temp = realloc(custom_keywords, (custom_keywords_count + 1) * sizeof(char*));
    if (temp) {
        custom_keywords = temp;
        custom_keywords[custom_keywords_count] = strdup(new_keyword);
        custom_keywords_count++;
    }

}

// funzione di utilità per concatenare stringhe con realloc
void append_str(char** dest, const char* src) {
    size_t new_len = (src ? strlen(src) : 0) + (*dest ? strlen(*dest) : 0) + 2;

    char* temp = realloc(*dest, new_len);
    if (!temp) return;

    if (*dest == NULL) {
        temp[0] = '\0';
    }

    *dest = temp;
    strcat(*dest, src);
    strcat(*dest, "\n");
}

//funzione per leggere un file, restituisce il buffer letto (NULL in caso di errore) e il valore del codice stato

typedef struct {
    char* buffer;
    StatusCode statusCode;
} FileRead;

FileRead filereader(char* const filedest) {

    // Inizializzo codice stato
    char*  buffer = NULL;
    StatusCode codiceStato = NO_ERROR;

    FILE *fp = fopen(filedest, "r");

    // Se non legge alcun dato segnalo errore di apertura ed esco
    if (!fp) {
        codiceStato = FILE_OPEN_ERROR;
        goto exitHandler;
    }

    // vado alla fine per capire quanto spazio allocare
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);

    //se ftell fallisce segnalo errore di lettura ed esco
    if (size == 0) {
        fclose(fp);
        codiceStato = FILE_READ_ERROR;
        goto exitHandler;
    }
    rewind(fp);

    // se malloc fallisce lo segnalo come un errore di lettura ed esco; tecnicamente sta fallendo la
    // funzione filereader stessa
    buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        codiceStato = FILE_READ_ERROR;
        goto exitHandler;
    }

    // Provo a mettere gli elementi letti nel buffer, se questo fallisce segnalo
    // errore di lettura ed esco
    if (fread(buffer, 1, size, fp) < size) {
        fclose(fp);
        free(buffer);
        buffer = NULL;
        codiceStato = FILE_READ_ERROR;
        goto exitHandler;
    }
    buffer[size] = '\0'; // chiudo la stringa

    // Se non riesco a chiudere il file segnalo l'errore chiusura file ed esco
    if (fclose(fp) != 0) codiceStato = FILE_CLOSE_ERROR;


    exitHandler:
    FileRead fileRead = {buffer, codiceStato};
    return fileRead;
}