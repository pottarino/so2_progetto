//
// Created by potta on 02/04/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* line;
    int line_number;
    char* filename;
} FileText;

typedef struct {
    FileText* headers;
    FileText* variables;
    FileText* main_body;
    int h_count, v_count, m_count;
} ParsedProgram;

void append_filetext(FileText** dest, int* count, const char* line, int line_number, const char* filename) {
    FileText* temp = realloc(*dest, (*count + 2) * sizeof(FileText));
    if (!temp) return;

    *dest = temp;
    (*dest)[*count].line = strdup(line);
    (*dest)[*count].line_number = line_number;
    (*dest)[*count].filename = strdup(filename);

    (*count)++;
    (*dest)[*count].line = NULL; // terminatore per sicurezza
}

ParsedProgram first_parsing(char * const text, char* const filename) {
    ParsedProgram p = {NULL, NULL, NULL, 0, 0, 0};
    char* copy = strdup(text);
    char* saveptr;
    char* riga = strtok_r(copy, "\n", &saveptr);

    int in_main = 0;
    int line_number = 1;

    while (riga != NULL) {
        // pulizia base spazi iniziali
        char* ptr = riga;
        while(*ptr == ' ' || *ptr == '\t') ptr++;

        // ignoro righe vuote o commenti semplici //
        if (ptr[0] == '\0' || strncmp(ptr, "//", 2) == 0) {
            riga = strtok_r(NULL, "\n", &saveptr);
            line_number++;
            continue;
        }

        //header
        if (ptr[0] == '#') {
            append_filetext(&p.headers, &p.h_count, ptr, line_number, filename);
        }
        // main?
        else if (strstr(ptr, "main") != NULL && strstr(ptr, "(") != NULL) {
            in_main = 1;
            append_filetext(&p.main_body, &p.m_count, ptr, line_number, filename);
        }
        //se sono nel main, aggiungo
        else if (in_main) {
            append_filetext(&p.main_body, &p.m_count, ptr, line_number, filename);
        }
        // non è header e non è main, per esclusione è una variabile globale
        else {
            append_filetext(&p.variables, &p.v_count, ptr, line_number, filename);
        }

        riga = strtok_r(NULL, "\n", &saveptr);
        line_number++;
    }

    free(copy);
    return p;
}

