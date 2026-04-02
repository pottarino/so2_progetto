#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
        custom_keywords[custom_keywords_count] = strdup(new_keyword
            );
        custom_keywords_count++;
    }

}



// funzione di utilità per concatenare stringhe con realloc
void append_str(char** dest, const char* src) {
    if (*dest == NULL) {
        *dest = strdup(src);
    } else {
        *dest = realloc(*dest, strlen(*dest) + strlen(src) + 2);
        strcat(*dest, src);
    }
    strcat(*dest, "\n");
}

//funzione per leggere un file
char* filereader(char* const filedest) {
    FILE *fp = fopen(filedest, "r");
    if (!fp) return NULL;

    // vado alla fine per capire quanto spazio allocare
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char* buffer = malloc(size + 1);
    if (buffer) {
        fread(buffer, 1, size, fp);
        buffer[size] = '\0'; // chiudo la stringa
    }

    fclose(fp);
    return buffer;
}