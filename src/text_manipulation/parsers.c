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
        int current_capacity = *count;

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

    // Uso realloc qui perché add_fragment non ha parametri per tracciare old_size esternamente
    // e viene chiamata molto frequentemente per singoli elementi
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
    //usiamo il filename contenuto nel primo FileText dell'array
    init_codeline(&current_cl, file_texts[0].filename);

    for (int i = 0; i < count; i++) {
        char* cursor = file_texts[i].line;
        int current_num = file_texts[i].line_number;
        char* semi;

        while ((semi = strchr(cursor, ';')) != NULL) {
            int len = (int)(semi - cursor);
            char* fragment = malloc(len + 1);
            strncpy(fragment, cursor, len);
            fragment[len] = '\0';

            add_fragment(&current_cl, fragment, current_num);
            free(fragment);

            //salvataggio dell'istruzione completata
            cl_count++;

            if (results == NULL) {
                allocated_size = sizeof(CodeLine) * 4;
                results = malloc(allocated_size);
            } else if (cl_count * sizeof(CodeLine) > allocated_size) {
                allocate_more((void**)&results, &allocated_size);
            }

            results[cl_count - 1] = current_cl;

            //prepariamo la prossima CodeLine
            init_codeline(&current_cl, file_texts[i].filename);

            cursor = semi + 1;
            while (*cursor == ' ' || *cursor == '\t') cursor++;
        }

        // Se la riga non finisce con ';', il resto va nella CodeLine corrente
        if (*cursor != '\0') {
            add_fragment(&current_cl, cursor, current_num);
        }
    }

    //gestione dell'ultimo frammento se il file non termina con ';'
    if (current_cl.count > 0) {
        cl_count++;
        if (cl_count * sizeof(CodeLine) > allocated_size) {
            results = realloc(results, sizeof(CodeLine) * cl_count);
        }
        results[cl_count - 1] = current_cl;
    }

    // Stringo alla dimensione reale prima di uscire
    free_unused((void**)&results, cl_count * sizeof(CodeLine));

    *out_cl_count = cl_count;
    return results;
}

/**
 * Libera la memoria interna di una struttura CodeLine.
 * Da usare quando una CodeLine viene scartata o non più necessaria.
 */
void free_codeline(CodeLine* cl) {
    if (cl == NULL) {
        return;
    }

    //libera ogni singola array line
    if (cl->lines != NULL) {
        for (int i = 0; i < cl->count; i++) {
            if (cl->lines[i] != NULL) {
                free(cl->lines[i]);
                cl->lines[i] = NULL;
            }
        }
        free(cl->lines);
        cl->lines = NULL;
    }

    // libera i numeri
    if (cl->line_numbers != NULL) {
        free(cl->line_numbers);
        cl->line_numbers = NULL;
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

    int total_cl = 0;
    // prendiamo le codeline
    CodeLine* all_instructions = split_into_codelines(p.variables, p.v_count, &total_cl);

    if (all_instructions == NULL) return pg;

    // vediamo le codeline
    for (int i = 0; i < total_cl; i++) {
        CodeLine current = all_instructions[i];
        int kept = 0;

        // typedef?
        if (current.count > 0 && strstr(current.lines[0], "typedef") != NULL) {
            pg.typedefs_count++;
            if (pg.typedefs == NULL) {
                type_alloc_size = sizeof(CodeLine) * 4;
                pg.typedefs = malloc(type_alloc_size);
            } else if (pg.typedefs_count * sizeof(CodeLine) > type_alloc_size) {
                allocate_more((void**)&pg.typedefs, &type_alloc_size);
            }
            pg.typedefs[pg.typedefs_count - 1] = current;
            kept = 1;
        }
        // allora è una variable declaration
        else {
            pg.variables_count++;
            if (pg.variable_lines == NULL) {
                var_alloc_size = sizeof(CodeLine) * 4;
                pg.variable_lines = malloc(var_alloc_size);
            } else if (pg.variables_count * sizeof(CodeLine) > var_alloc_size) {
                allocate_more((void**)&pg.variable_lines, &var_alloc_size);
            }
            pg.variable_lines[pg.variables_count - 1] = current;
            kept = 1;
        }
    }

    // Stringo entrambi alla fine
    free_unused((void**)&pg.typedefs, pg.typedefs_count * sizeof(CodeLine));
    free_unused((void**)&pg.variable_lines, pg.variables_count * sizeof(CodeLine));

    free(all_instructions);
    return pg;
}


// Questa funzione formatta una CodeLine e la restituisce wrappata in una ParsedCodeLine contenente
// sia la CodeLine originale inviolata che la stringa ottenuta parsandone il contenuto
ParsedCodeLine parseCodeLine(CodeLine const c) {
    ParsedCodeLine parsedCodeLine;
    parsedCodeLine.codeLine = c;
    // Lo stavate aspettando! Ci ho messo più di 4 ore a scrivere questa stupidaggine
    int dimensioneTotale = 0;
    char* stringaFormattata = malloc(1);
    stringaFormattata[0] = '\0';
    for (int contatoreRiga = 0; contatoreRiga < c.count; contatoreRiga++ ) {
        // Ho una riga non formattata. Devo formattarla

        // Prima passata:
        // Controllo il numero di caratteri "reali" che ci interessano
        // (gne gne gne è un puntatore non è un array gne gne gne tipico C# loser)
        int numeroCaratteriRiga = 0;
        char* start = c.lines[contatoreRiga];
        char* end = start;

        // Imposto le flags
        bool flagStringa = false;
        while ( *end != '\0') {
            char carattereCorrente = *end;
            if (carattereCorrente == ' ' && flagStringa == false) {
                if (*(end+1) == ' ' || *(end+1) == '\0') {
                    end++;
                    continue;
                }
            }
            if ((carattereCorrente == '\t' || carattereCorrente == '\n') && flagStringa == false) {
                end++;
                continue;
            }
            if (carattereCorrente == '"') {
                if (flagStringa == false) flagStringa = true;
                else flagStringa = false;
            }
            numeroCaratteriRiga++;
            end++;
        }

        // Alloco la nuova riga
        char* rigaFormattata = malloc(numeroCaratteriRiga+1);

        // Seconda passata per aggiungere caratteri alla rigaFormattata
        int contatoreCarattere = 0;
        end = start;
        flagStringa = false;
        while ( *end != '\0') {
            char carattereCorrente = *end;
            if (carattereCorrente == ' ' && flagStringa == false) {
                if (*(end+1) == ' ' || *(end+1) == '\0') {
                    end++;
                    continue;
                }
            }
            if ((carattereCorrente == '\t' || carattereCorrente == '\n') && flagStringa == false) {
                end++;
                continue;
            }
            if (carattereCorrente == '"') {
                if (flagStringa == false) flagStringa = true;
                else flagStringa = false;
            }
            rigaFormattata[contatoreCarattere] = *end;
            contatoreCarattere++;
            end++;
        }
        // Una volta avuta la riga formattata la concateno alla stringa usando una malloc
        dimensioneTotale += numeroCaratteriRiga;
        rigaFormattata[numeroCaratteriRiga] = '\0';
        // Se non sono nella prima iterazione, devo aggiungere degli spazi e quindi cambia la dimensione della realloc
        if (contatoreRiga != 0) {
            dimensioneTotale += 1;
            stringaFormattata = realloc(stringaFormattata, dimensioneTotale +1 );
            strcat(stringaFormattata, " ");
            stringaFormattata =  strcat(stringaFormattata, rigaFormattata);
        }
        else {
            stringaFormattata = realloc(stringaFormattata, dimensioneTotale +1 );
            stringaFormattata =  strcat(stringaFormattata, rigaFormattata);
        }
        free(rigaFormattata);
    }
    // Aggiornare il valore di stringaFormattata della struct
    parsedCodeLine.formattedCodeLine = stringaFormattata;

    // Restituire la struct; printf("Hallelujah!")
    return parsedCodeLine;
}

ParsedMain parseMainProgram(ParsedProgram pp) {
    int codeline_count = 0;
    CodeLine* main_instructions = split_into_codelines(pp.main_body, pp.m_count,&codeline_count);
    ParsedMain pm = {NULL, 0, NULL, 0};

    int var_alloc_size = 0;
    int inst_alloc_size = 0;

    // il main a quanto ho capito dovrebbe essere diviso solo ed esclusivamente in variabili e istruzioni di altro tipo
    for (int i = 0; i < codeline_count; i++) {
        CodeLine current_cl = main_instructions[i];
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

    // Stringo alla dimensione reale
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
        const char* current_line = program->headers[i].line;

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

// secca tutto il ParsedProgram
void free_parsed_program(ParsedProgram* p) {
    if (!p) return;
    // pulizia header, variabili e main
    for (int i = 0; i < p->h_count; i++) {
        free(p->headers[i].line);
        free(p->headers[i].filename);
    }
    for (int i = 0; i < p->v_count; i++) {
        free(p->variables[i].line);
        free(p->variables[i].filename);
    }
    for (int i = 0; i < p->m_count; i++) {
        free(p->main_body[i].line);
        free(p->main_body[i].filename);
    }
    free(p->headers);
    free(p->variables);
    free(p->main_body);
}

// killa ParsedGlobal e ParsedMain (che usano CodeLine)
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

// libera i nomi degli header
void free_headers(ParsedHeaders* ph) {
    if (!ph || !ph->headers) return;
    for (int i = 0; i < ph->count; i++) {
        if (ph->headers[i]) free(ph->headers[i]);
    }
    free(ph->headers);
}

