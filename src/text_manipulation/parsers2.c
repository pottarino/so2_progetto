#include <ctype.h>
#include "parsers.h"
#include <stdlib.h>
#include <string.h>

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    char *copy = malloc(strlen(s) + 1);
    if (copy) strcpy(copy, s);
    return copy;
}



void append_to_codeline(CodeLine *cl, char *line, int line_n) {
    cl->lines        = realloc(cl->lines,        (cl->count + 1) * sizeof(char *));
    cl->line_numbers = realloc(cl->line_numbers, (cl->count + 1) * sizeof(int));
    cl->lines[cl->count]        = strdup_safe(line);
    cl->line_numbers[cl->count] = line_n;
    cl->count++;
}

int could_be_main_dec(char *line) {
    if (!line) return 0;
    char *m = strstr(line, "main");
    if (!m) return 0;
    char *p = strchr(m, '(');
    char *b = strchr(m, '{');
    return (p != NULL || b != NULL);
}

int is_main_dec(char *line) {
    if (!line) return 0;
    if (!could_be_main_dec(line)) return 0;
    char buf[1024];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *p = buf; //buffer
    while (*p == ' ' || *p == '\t') p++; //skippo gli spazi vuoti
    if (strncmp(p, "int",  3) == 0 && (p[3]==' '||p[3]=='\t')) p += 3; //cerco di ricostruire
    else if (strncmp(p, "void", 4) == 0 && (p[4]==' '||p[4]=='\t')) p += 4;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "main", 4) != 0) return 0;
    p += 4;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '(') return 0;
    return 1;
}

void append_to_parsed_program(ParsedProgram *pp, CodeLine cl, int phase, char *line) {
    ParsedCodeLine pcl = parseCodeLine(cl);
    ParsedCodeLine **target_array;
    int            *target_count;

    switch (phase) {
        case 0:
            target_array = &pp->headers;
            target_count = &pp->h_count;
            break;
        case 1:
            target_array = &pp->variables;
            target_count = &pp->v_count;
            break;
        case 2:
        default:
            target_array = &pp->main_body;
            target_count = &pp->m_count;
            break;
    }

    *target_array = realloc(*target_array, (*target_count + 1) * sizeof(ParsedCodeLine));
    (*target_array)[*target_count] = pcl;
    (*target_count)++;
}

ParsedProgram parse_program(const char *text, char *filename) {
    ParsedProgram pp = {NULL, NULL, NULL, filename, 0, 0, 0};
    int in_comment = 0;
    int in_multi_comment = 0;
    int exiting_multi_comment = 0;
    int entering_comment = 0;
    int in_string = 0;
    int line_count = 0;
    int used_space = 0;
    int phase = 0;
    int possibly_update_phase = 1;
    int current_size = 10;
    char *line = malloc(current_size);
    if (!line) return pp;
    line[0] = '\0';

    CodeLine cl = {NULL, NULL, 0, filename};
    int chars = (int)strlen(text);

    for (int char_index = 0; char_index < chars; char_index++) {
        if (possibly_update_phase == 1) {
            switch (phase) {
                case 0:
                    if (used_space > 0 && line[0] != '#') phase = 1;
                    possibly_update_phase = 0;
                    break;
                default:
                    if (!could_be_main_dec(line))
                        possibly_update_phase = 1;
                    if (is_main_dec(line)) {
                        possibly_update_phase = 0;
                        phase = 2;
                        cl.lines = NULL;
                        cl.line_numbers = NULL;
                        cl.count = 0;
                        used_space = 0;
                        line[0] = '\0';
                        while (char_index < chars && text[char_index] != '{') {
                            if (text[char_index] == '\n') line_count++;
                            char_index++;
                        }
                        if (char_index < chars) char_index++;
                    }
                    break;
            }
            if (char_index >= chars) break;
        }

        if (text[char_index] == '\n') {
            in_comment = 0;
            line_count++;
            continue;
        }

        if (text[char_index] == '"' && !in_comment && !in_multi_comment) {
            in_string = !in_string;
        }

        if (text[char_index] == '/' && !in_string && !in_comment && !in_multi_comment && !entering_comment) {
            entering_comment = 1;
            continue;
        }

        if (in_comment) continue;

        if (in_multi_comment) {
            if (exiting_multi_comment && text[char_index] == '/') {
                in_multi_comment = 0;
                exiting_multi_comment = 0;
            } else if (exiting_multi_comment) {
                exiting_multi_comment = 0;
            }
            if (text[char_index] == '*') exiting_multi_comment = 1;
            continue;
        }

        if (entering_comment) {
            if (text[char_index] == '*') in_multi_comment = 1;
            else if (text[char_index] == '/') in_comment = 1;
            else {
                if (used_space + 2 >= current_size) {
                    current_size *= 2;
                    char *temp = realloc(line, current_size);
                    if (temp) line = temp;
                }
                line[used_space++] = '/';
                line[used_space] = '\0';
            }
            entering_comment = 0;
            if (in_comment || in_multi_comment) continue;
        }

        if (text[char_index] == ';') {
            if (used_space + 2 >= current_size) {
                current_size *= 2;
                char *temp = realloc(line, current_size);
                if (temp) line = temp;
            }
            line[used_space++] = ';';
            line[used_space] = '\0';

            append_to_codeline(&cl, line, line_count);
            append_to_parsed_program(&pp, cl, phase, line);

            cl.lines = NULL;
            cl.line_numbers = NULL;
            cl.count = 0;
            used_space = 0;
            line[0] = '\0';

            if (phase < 2) possibly_update_phase = 1;
            continue;
        }

        if (used_space + 2 >= current_size) {
            current_size *= 2;
            char *temp = realloc(line, current_size);
            if (temp) line = temp;
        }
        line[used_space++] = text[char_index];
        line[used_space] = '\0';
    }
    printf("Parsing completato. Headers: %d, Vars: %d, Main: %d\n", pp.h_count, pp.v_count, pp.m_count);
    fflush(stdout);
    free(line);
    return pp;
}