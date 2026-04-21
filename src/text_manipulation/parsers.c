//
// Created by potta on 02/04/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "variable_recognizer.h"

void append_filetext(FileText** dest, int* count, const char* line, int line_number, const char* filename) {
    // Inizializzazione
    if (*dest == NULL) {
        int initial_size = sizeof(FileText) * 4; // Partiamo con 4 slot
        *dest = malloc(initial_size);
        if (!*dest) return;
    }
    else {
        FileText* temp = realloc(*dest, (*count + 2) * sizeof(FileText));
        if (!temp) return;
        *dest = temp;
    }

    (*dest)[*count].line = strdup(line);
    (*dest)[*count].line_number = line_number;
    (*dest)[*count].filename = strdup(filename);

    (*count)++;

    // terminatore null
    (*dest)[*count].line = NULL;
}

ParsedProgram first_parsing(char * const text, char* const filename) {
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

    cl->lines = realloc(cl->lines, sizeof(char*) * cl->count);
    cl->line_numbers = realloc(cl->line_numbers, sizeof(int) * cl->count);

    cl->lines[cl->count - 1] = strdup(text);
    cl->line_numbers[cl->count - 1] = line_num;
}

CodeLine* split_into_codelines(const FileText* file_texts, const int count, int* out_cl_count) {
    if (count <= 0 || file_texts == NULL) {
        *out_cl_count = 0;
        return NULL;
    }

    int cl_count = 0;
    CodeLine* results = NULL;
    int allocated_size = 0;

    CodeLine current_cl;
    init_codeline(&current_cl, file_texts[0].filename);

    for (int i = 0; i < count; i++) {
        char* cursor = file_texts[i].line;
        int current_num = file_texts[i].line_number;

        if (!cursor || cursor[0] == '\0') continue;

        char* semi;
        while ((semi = strchr(cursor, ';')) != NULL) {
            int len = (int)(semi - cursor);

            // Estraiamo il frammento fino al ';'
            char* fragment = malloc(len + 1);
            strncpy(fragment, cursor, len);
            fragment[len] = '\0';

            // Pulizia spazi iniziali del frammento
            char* clean_frag = fragment;
            while (*clean_frag == ' ' || *clean_frag == '\t') clean_frag++;

            if (clean_frag[0] != '\0') {
                add_fragment(&current_cl, clean_frag, current_num);
            }
            free(fragment);

            // Se abbiamo almeno un frammento, chiudiamo questa CodeLine e salviamola
            if (current_cl.count > 0) {
                cl_count++;
                if (results == NULL) {
                    allocated_size = sizeof(CodeLine) * 4;
                    results = malloc(allocated_size);
                } else if (cl_count * sizeof(CodeLine) > allocated_size) {
                    allocate_more((void**)&results, &allocated_size);
                }

                results[cl_count - 1] = current_cl;

                // Reset per la prossima istruzione
                init_codeline(&current_cl, file_texts[i].filename);
            }

            cursor = semi + 1;
            while (*cursor == ' ' || *cursor == '\t') cursor++;
        }

        // Se dopo l'ultimo ';' della riga rimane del testo (es: inizio di una nuova dichiarazione)
        // lo aggiungiamo alla CodeLine corrente che verrà chiusa dal ';' nella riga successiva
        if (*cursor != '\0') {
            add_fragment(&current_cl, cursor, current_num);
        }
    }

    // Gestione dell'ultimo frammento se il file non termina con ';'
    if (current_cl.count > 0) {
        cl_count++;
        if (cl_count * sizeof(CodeLine) > allocated_size) {
            results = realloc(results, sizeof(CodeLine) * cl_count);
        }
        results[cl_count - 1] = current_cl;
    } else {
        // Se l'ultima codeline è rimasta vuota (es: file finisce con ;), liberiamo il filename duplicato
        if (current_cl.filename) free(current_cl.filename);
    }

    if (cl_count > 0) {
        free_unused((void**)&results, cl_count * sizeof(CodeLine));
    }

    *out_cl_count = cl_count;
    return results;
}

/**
 * Libera la memoria interna di una struttura CodeLine.
 * Da usare quando una CodeLine viene scartata o non più necessaria.
 */
void free_codeline(CodeLine* cl) {
    if (cl == NULL) return;

    if (cl->lines != NULL) {
        for (int i = 0; i < cl->count; i++) {
            if (cl->lines[i] != NULL) free(cl->lines[i]);
        }
        free(cl->lines);
        cl->lines = NULL;
    }

    if (cl->line_numbers != NULL) {
        free(cl->line_numbers);
        cl->line_numbers = NULL;
    }

    if (cl->filename != NULL) {
        free(cl->filename);
        cl->filename = NULL;
    }

    cl->count = 0;
}

ParsedGlobal parseGlobal(ParsedProgram p) {
    ParsedGlobal pg;
    pg.variable_lines = NULL;
    pg.typedefs = NULL;
    pg.variables_count = 0;
    pg.typedefs_count = 0;

    int var_alloc_size = 0;
    int type_alloc_size = 0;

    int total_cl = p.v_count;
    ParsedCodeLine* all_instructions = p.variables;

    if (all_instructions == NULL) return pg;

    for (int i = 0; i < total_cl; i++) {
        CodeLine current = all_instructions[i].codeLine;

        if (current.count > 0 && strstr(current.lines[0], "typedef") != NULL) {
            pg.typedefs_count++;
            if (pg.typedefs == NULL) {
                type_alloc_size = sizeof(CodeLine) * 4;
                pg.typedefs = malloc(type_alloc_size);
            } else if (pg.typedefs_count * sizeof(CodeLine) > type_alloc_size) {
                allocate_more((void**)&pg.typedefs, &type_alloc_size);
            }
            pg.typedefs[pg.typedefs_count - 1] = current;
        }
        else {
            pg.variables_count++;
            if (pg.variable_lines == NULL) {
                var_alloc_size = sizeof(CodeLine) * 4;
                pg.variable_lines = malloc(var_alloc_size);
            } else if (pg.variables_count * sizeof(CodeLine) > var_alloc_size) {
                allocate_more((void**)&pg.variable_lines, &var_alloc_size);
            }
            pg.variable_lines[pg.variables_count - 1] = current;
        }
    }

    free_unused((void**)&pg.typedefs, pg.typedefs_count * sizeof(CodeLine));
    free_unused((void**)&pg.variable_lines, pg.variables_count * sizeof(CodeLine));

    free(all_instructions);
    return pg;
}

ParsedCodeLine parseCodeLine(CodeLine const c) {
    ParsedCodeLine parsedCodeLine;
    parsedCodeLine.codeLine = c;

    int dimensioneTotale = 0;
    char* stringaFormattata = malloc(1);
    if (!stringaFormattata) return parsedCodeLine;
    stringaFormattata[0] = '\0';

    for (int contatoreRiga = 0; contatoreRiga < c.count; contatoreRiga++) {
        char* start = c.lines[contatoreRiga];
        if (!start) continue;

        int numeroCaratteriRiga = 0;
        bool flagStringa = false;
        for (int i = 0; start[i] != '\0'; i++) {
            if (!flagStringa) {
                if (start[i] == '\t' || start[i] == '\n' || start[i] == '\r') continue;
                if (start[i] == ' ' && (start[i+1] == ' ' || start[i+1] == '\0' || start[i+1] == '\r')) continue;
            }
            if (start[i] == '"') flagStringa = !flagStringa;
            numeroCaratteriRiga++;
        }

        char* rigaFormattata = malloc(numeroCaratteriRiga + 1);
        if (!rigaFormattata) continue;

        int k = 0;
        flagStringa = false;
        for (int i = 0; start[i] != '\0'; i++) {
            if (!flagStringa) {
                if (start[i] == '\t' || start[i] == '\n' || start[i] == '\r') continue;
                if (start[i] == ' ' && (start[i+1] == ' ' || start[i+1] == '\0' || start[i+1] == '\r')) continue;
            }
            if (start[i] == '"') flagStringa = !flagStringa;
            rigaFormattata[k++] = start[i];
        }
        rigaFormattata[k] = '\0';

        int spazioNecessario = dimensioneTotale + numeroCaratteriRiga + (contatoreRiga > 0 ? 1 : 0) + 1;
        char* temp = realloc(stringaFormattata, spazioNecessario);
        if (!temp) {
            free(rigaFormattata);
            break;
        }
        stringaFormattata = temp;

        if (contatoreRiga > 0) strcat(stringaFormattata, " ");
        strcat(stringaFormattata, rigaFormattata);

        dimensioneTotale = strlen(stringaFormattata);
        free(rigaFormattata);
    }

    parsedCodeLine.formattedCodeLine = stringaFormattata;
    return parsedCodeLine;
}

ParsedMain parseMainProgram(ParsedProgram pp) {
    int codeline_count = pp.m_count;
    ParsedCodeLine* main_instructions = pp.main_body;
    ParsedMain pm = {NULL, 0, NULL, 0};

    int var_alloc_size = 0;
    int inst_alloc_size = 0;

    for (int i = 0; i < codeline_count; i++) {
        CodeLine current_cl = main_instructions[i].codeLine;
        if (is_variable(current_cl)) {
            pm.variables_count++;
            if (pm.variable_lines == NULL) {
                var_alloc_size = sizeof(CodeLine) * 4;
                pm.variable_lines = malloc(var_alloc_size);
            } else if (pm.variables_count * sizeof(CodeLine) > var_alloc_size) {
                allocate_more((void**)&pm.variable_lines, &var_alloc_size);
            }
            pm.variable_lines[pm.variables_count - 1] = current_cl;
        }
        else {
            pm.instructions_count++;
            if (pm.instructions == NULL) {
                inst_alloc_size = sizeof(CodeLine) * 4;
                pm.instructions = malloc(inst_alloc_size);
            } else if (pm.instructions_count * sizeof(CodeLine) > inst_alloc_size) {
                allocate_more((void**)&pm.instructions, &inst_alloc_size);
            }
            pm.instructions[pm.instructions_count - 1] = current_cl;
        }
    }
    free_unused((void**)&pm.variable_lines, pm.variables_count * sizeof(CodeLine));
    free_unused((void**)&pm.instructions, pm.instructions_count * sizeof(CodeLine));

    return pm;
}

ParsedHeaders parseHeaders(ParsedProgram* program) {
    ParsedHeaders result;
    result.count = program->h_count;
    result.headers = (char**)malloc(result.count * sizeof(char*));

    if (!result.headers) {
        result.count = 0;
        return result;
    }

    for (int i = 0; i < program->h_count; i++) {
        const char* current_line = program->headers[i].formattedCodeLine;
        const char* start = strpbrk(current_line, "<\"");
        const char* end = NULL;

        if (start) {
            char target = (*start == '<') ? '>' : '"';
            end = strchr(start + 1, target);
        }

        if (start && end) {
            size_t len = end - start - 1;
            result.headers[i] = (char*)malloc(len + 1);
            if (result.headers[i]) {
                strncpy(result.headers[i], start + 1, len);
                result.headers[i][len] = '\0';
            }
        } else {
            result.headers[i] = NULL;
        }
    }
    return result;
}


void free_parsed_parts(ParsedGlobal* pg, ParsedMain* pm) {
    if (pg) {
        for (int i = 0; i < pg->variables_count; i++) free_codeline(&pg->variable_lines[i]);
        for (int i = 0; i < pg->typedefs_count; i++) free_codeline(&pg->typedefs[i]);
        free(pg->variable_lines);
        free(pg->typedefs);
    }
    if (pm) {
        for (int i = 0; i < pm->variables_count; i++) free_codeline(&pm->variable_lines[i]);
        for (int i = 0; i < pm->instructions_count; i++) free_codeline(&pm->instructions[i]);
        free(pm->variable_lines);
        free(pm->instructions);
    }
}

void free_headers(ParsedHeaders* ph) {
    if (!ph || !ph->headers) return;
    for (int i = 0; i < ph->count; i++) {
        if (ph->headers[i]) free(ph->headers[i]);
    }
    free(ph->headers);
}

