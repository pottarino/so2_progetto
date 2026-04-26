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

// CodeLine* split_into_codelines(const FileText* file_texts, const int count, int* out_cl_count) {
//     if (count <= 0 || file_texts == NULL) {
//         *out_cl_count = 0;
//         return NULL;
//     }
//
//     int cl_count = 0;
//     CodeLine* results = NULL;
//     int allocated_size = 0;
//
//     CodeLine current_cl;
//     init_codeline(&current_cl, file_texts[0].filename);
//
//     for (int i = 0; i < count; i++) {
//         char* cursor = file_texts[i].line;
//         int current_num = file_texts[i].line_number;
//
//         if (!cursor || cursor[0] == '\0') continue;
//
//         char* semi;
//         while ((semi = strchr(cursor, ';')) != NULL) {
//             int len = (int)(semi - cursor);
//
//             // Estraiamo il frammento fino al ';'
//             char* fragment = malloc(len + 1);
//             strncpy(fragment, cursor, len);
//             fragment[len] = '\0';
//
//             // Pulizia spazi iniziali del frammento
//             char* clean_frag = fragment;
//             while (*clean_frag == ' ' || *clean_frag == '\t') clean_frag++;
//
//             if (clean_frag[0] != '\0') {
//                 add_fragment(&current_cl, clean_frag, current_num);
//             }
//             free(fragment);
//
//             // Se abbiamo almeno un frammento, chiudiamo questa CodeLine e salviamola
//             if (current_cl.count > 0) {
//                 cl_count++;
//                 if (results == NULL) {
//                     allocated_size = sizeof(CodeLine) * 4;
//                     results = malloc(allocated_size);
//                 } else if (cl_count * sizeof(CodeLine) > allocated_size) {
//                     allocate_more((void**)&results, &allocated_size);
//                 }
//
//                 results[cl_count - 1] = current_cl;
//
//                 // Reset per la prossima istruzione
//                 init_codeline(&current_cl, file_texts[i].filename);
//             }
//
//             cursor = semi + 1;
//             while (*cursor == ' ' || *cursor == '\t') cursor++;
//         }
//
//         // Se dopo l'ultimo ';' della riga rimane del testo (es: inizio di una nuova dichiarazione)
//         // lo aggiungiamo alla CodeLine corrente che verrà chiusa dal ';' nella riga successiva
//         if (*cursor != '\0') {
//             add_fragment(&current_cl, cursor, current_num);
//         }
//     }
//
//     // Gestione dell'ultimo frammento se il file non termina con ';'
//     if (current_cl.count > 0) {
//         cl_count++;
//         if (cl_count * sizeof(CodeLine) > allocated_size) {
//             results = realloc(results, sizeof(CodeLine) * cl_count);
//         }
//         results[cl_count - 1] = current_cl;
//     } else {
//         // Se l'ultima codeline è rimasta vuota (es: file finisce con ;), liberiamo il filename duplicato
//         if (current_cl.filename) free(current_cl.filename);
//     }
//
//     if (cl_count > 0) {
//         free_unused((void**)&results, cl_count * sizeof(CodeLine));
//     }
//
//     *out_cl_count = cl_count;
//     return results;
// }

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