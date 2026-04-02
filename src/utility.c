#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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