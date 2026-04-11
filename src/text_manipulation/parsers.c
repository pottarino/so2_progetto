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
    char* filename;
    int h_count, v_count, m_count;
} ParsedProgram;

typedef struct {
    char** lines;
    int* line_numbers;
    int count;
    char* filename;
} CodeLine;


typedef struct {
    CodeLine *variable_lines;
    CodeLine *instructions;
} ParsedMain;

typedef struct {
    CodeLine* variable_lines;
    CodeLine* typedefs;
    int variables_count;
    int typedefs_count;
}ParsedGlobal;

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
    ParsedProgram p = {NULL, NULL, NULL, NULL, 0, 0, 0};
    char* copy = strdup(text);
    char* saveptr;
    char* riga = strtok_r(copy, "\n", &saveptr);

    int in_main = 0;
    int in_comment = 0;
    int line_num = 1;

    while (riga != NULL) {
        char* ptr = riga;

        // se siamo dentro un commento /* o ne inizia uno qui, puliamo tutto
        char* start;
        while ((start = strstr(ptr, in_comment ? "" : "/*")) != NULL || in_comment) {
            if (!in_comment) {
                char* end = strstr(start, "*/");
                if (end) {
                    memset(start, ' ', (end + 2) - start);
                    continue;
                } else {
                    *start = '\0';
                    in_comment = 1;
                    break;
                }
            } else {
                char* end = strstr(ptr, "*/");
                if (end) {
                    memset(ptr, ' ', (end + 2) - ptr);
                    in_comment = 0;
                    continue;
                } else {
                    *ptr = '\0';
                    break;
                }
            }
        }

        // secca i commenti inline // ovunque siano
        char* inline_comment = strstr(ptr, "//");
        if (inline_comment) {
            *inline_comment = '\0';
        }

        // togliamo gli spazi bianchi rimasti a inizio riga
        while(*ptr == ' ' || *ptr == '\t' || *ptr == '\r') ptr++;

        // se la riga è vuota dopo la pulizia, passiamo alla prossima ma contiamo la riga
        if (ptr[0] == '\0') {
            riga = strtok_r(NULL, "\n", &saveptr);
            line_num++;
            continue;
        }

        // ora che la riga è "nuda", decidiamo dove schiaffarla
        if (ptr[0] == '#') {
            append_filetext(&p.headers, &p.h_count, ptr, line_num, filename);
        }
        else if (strstr(ptr, "main") && strstr(ptr, "(")) {
            in_main = 1;
            append_filetext(&p.main_body, &p.m_count, ptr, line_num, filename);
        }
        else if (in_main) {
            append_filetext(&p.main_body, &p.m_count, ptr, line_num, filename);
        }
        else {
            append_filetext(&p.variables, &p.v_count, ptr, line_num, filename);
        }

        riga = strtok_r(NULL, "\n", &saveptr);
        line_num++;
    }

    free(copy);
    p.filename = filename;
    return p;
}

// inizializza una codeline vuota
void init_codeline(CodeLine* cl, const char* filename) {
    cl->lines = NULL;
    cl->line_numbers = NULL;
    cl->count = 0;
    cl->filename = strdup(filename);
}

// aggiunge un pezzo di codice alla struttura corrente
void add_fragment(CodeLine* cl, const char* text, int line_num) {
    cl->count++;

    // alloco/rialloco tutto il blocco per il nuovo pezzo
    cl->lines = realloc(cl->lines, sizeof(char*) * cl->count);
    cl->line_numbers = realloc(cl->line_numbers, sizeof(int) * cl->count);

    cl->lines[cl->count - 1] = strdup(text);
    cl->line_numbers[cl->count - 1] = line_num;
}

ParsedGlobal parseGlobal(ParsedProgram p) {
    //inizializziamo
    ParsedGlobal pg;
    pg.variable_lines = NULL;
    pg.typedefs = NULL;
    int v_count = 0;
    int t_count = 0;

    CodeLine current_cl;
    init_codeline(&current_cl, p.filename);

    for (int i = 0; i < p.v_count; i++) {
        char* cursor = p.variables[i].line;
        int current_num = p.variables[i].line_number;

        char* semi;
        // finché troviamo un punto e virgola, l'istruzione corrente "chiude"
        while ((semi = strchr(cursor, ';')) != NULL) {
            // estraiamo il pezzo di codice prima del ;
            int len = (int)(semi - cursor);
            char* fragment = malloc(len + 1);
            strncpy(fragment, cursor, len);
            fragment[len] = '\0';

            // aggiungiamo l'ultimo pezzo a current_cl
            add_fragment(&current_cl, fragment, current_num);
            free(fragment);

            // ora che è chiusa col ;, decidiamo dove salvarla.
            // dato che i commenti non ci sono, il primo frammento (lines[0])
            // deve per forza contenere "typedef" se è un typedef.
            if (current_cl.count > 0 && strstr(current_cl.lines[0], "typedef") != NULL) {
                t_count++;
                pg.typedefs = realloc(pg.typedefs, sizeof(CodeLine) * t_count);
                pg.typedefs[t_count - 1] = current_cl;
            } else {
                v_count++;
                pg.variable_lines = realloc(pg.variable_lines, sizeof(CodeLine) * v_count);
                pg.variable_lines[v_count - 1] = current_cl;
            }

            // pulizia: resettiamo current_cl per la prossima istruzione logica
            init_codeline(&current_cl, p.filename);

            // spostiamo il cursore oltre il ; e saltiamo eventuali spazi
            cursor = semi + 1;
            while (*cursor == ' ' || *cursor == '\t') cursor++;
        }

        // se siamo qui e il cursore non è alla fine della riga, significa che
        // quello che resta è l'inizio di un'istruzione che continua alla riga dopo
        if (*cursor != '\0') {
            add_fragment(&current_cl, cursor, current_num);
        }
    }

    // se il file finisce e abbiamo ancora roba in canna (magari manca un ; finale)
    if (current_cl.count > 0) {
        if (strstr(current_cl.lines[0], "typedef") != NULL) {
            t_count++;
            pg.typedefs = realloc(pg.typedefs, sizeof(CodeLine) * t_count);
            pg.typedefs[t_count - 1] = current_cl;
        } else {
            v_count++;
            pg.variable_lines = realloc(pg.variable_lines, sizeof(CodeLine) * v_count);
            pg.variable_lines[v_count - 1] = current_cl;
        }
    }

    pg.typedefs_count = t_count;
    pg.variables_count = v_count;
    return pg;
}

