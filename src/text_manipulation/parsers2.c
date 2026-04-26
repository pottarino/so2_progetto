#include <ctype.h>
#include "parsers.h"
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "variable_recognizer.h"

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
    char *p = buf;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (strncmp(p, "int",  3) == 0 && (p[3]==' '||p[3]=='\t')) p += 3;
    else if (strncmp(p, "void", 4) == 0 && (p[4]==' '||p[4]=='\t')) p += 4;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (strncmp(p, "main", 4) != 0) return 0;
    p += 4;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p != '(') return 0;
    return 1;
}

static int is_control_block(const char *line) {
    if (!line) return 0;
    const char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (strncmp(p, "for",  3) == 0 && (p[3]==' '||p[3]=='(')) return 1;
    if (strncmp(p, "while",5) == 0 && (p[5]==' '||p[5]=='(')) return 1;
    if (strncmp(p, "if",   2) == 0 && (p[2]==' '||p[2]=='(')) return 1;
    if (strncmp(p, "else", 4) == 0 && (p[4]==' '||p[4]=='{'||p[4]=='\0')) return 1;
    if (strncmp(p, "do",   2) == 0 && (p[2]==' '||p[2]=='{')) return 1;
    if (strstr(line, "else")) return 1;
    return 0;
}

void append_to_parsed_program(ParsedProgram *pp, CodeLine cl, int phase, char *line) {
    CodeLine cl_copy;
    cl_copy.filename = cl.filename;
    cl_copy.count = cl.count;
    cl_copy.lines = malloc(cl.count * sizeof(char *));
    cl_copy.line_numbers = malloc(cl.count * sizeof(int));
    for (int i = 0; i < cl.count; i++) {
        cl_copy.lines[i] = cl.lines[i] ? strdup(cl.lines[i]) : NULL;
        cl_copy.line_numbers[i] = cl.line_numbers[i];
    }

    ParsedCodeLine pcl = parseCodeLine(cl_copy);
    pcl.codeLine = cl_copy;

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

static void reset_cl(CodeLine *cl) {
    free(cl->lines);
    free(cl->line_numbers);
    cl->lines = NULL;
    cl->line_numbers = NULL;
    cl->count = 0;
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
    int brace_depth = 0;

    // stack per phase 2 — traccia se ogni {} e' un blocco di controllo
    int control_stack[256];
    int control_top = 0;
    memset(control_stack, 0, sizeof(control_stack));

    // in phase 0/1 tracciamo la profondita' delle graffe per saltare il corpo dei typedef struct
    int global_brace_depth = 0;

    for (int char_index = 0; char_index < chars; char_index++) {

        if (text[char_index] == '\r') continue;

        // gestione {} in phase 0/1 per typedef struct — accumula tutto come unico token
        if (phase < 2 && !in_string && !in_comment && !in_multi_comment) {
            if (text[char_index] == '{') {
                global_brace_depth++;
                if (used_space + 2 >= current_size) {
                    current_size *= 2;
                    char *tmp = realloc(line, current_size);
                    if (tmp) line = tmp;
                }
                line[used_space++] = '{';
                line[used_space] = '\0';
                continue;
            }
            if (text[char_index] == '}' && global_brace_depth > 0) {
                global_brace_depth--;
                if (used_space + 2 >= current_size) {
                    current_size *= 2;
                    char *tmp = realloc(line, current_size);
                    if (tmp) line = tmp;
                }
                line[used_space++] = '}';
                line[used_space] = '\0';
                // se siamo tornati a depth 0 il ; che segue chiudera' il typedef
                continue;
            }
            // se siamo dentro un blocco globale, accumula ma non processare
            if (global_brace_depth > 0) {
                if (text[char_index] == '\n') {
                    in_comment = 0;
                    line_count++;
                    continue;
                }
                if (used_space + 2 >= current_size) {
                    current_size *= 2;
                    char *tmp = realloc(line, current_size);
                    if (tmp) line = tmp;
                }
                line[used_space++] = text[char_index];
                line[used_space] = '\0';
                continue;
            }
        }

        if (possibly_update_phase && brace_depth == 0) {
            switch (phase) {
                case 0:
                    if (used_space == 0) break;
                    if (line[0] != '#') phase = 1;
                    else possibly_update_phase = 0;
                    break;
                default:
                    if (used_space == 0) break;
                    if (is_main_dec(line)) {
                        phase = 2;
                        possibly_update_phase = 0;
                        reset_cl(&cl);
                        used_space = 0;
                        line[0] = '\0';
                        while (char_index < chars && text[char_index] != '{') {
                            if (text[char_index] == '\n') line_count++;
                            char_index++;
                        }
                    }
                    break;
            }
            if (char_index >= chars) break;
        }

        if (text[char_index] == '\n') {
            in_comment = 0;
            line_count++;
            if (phase == 0 && used_space > 0 && line[0] == '#') {
                append_to_codeline(&cl, line, line_count);
                append_to_parsed_program(&pp, cl, phase, line);
                reset_cl(&cl);
                used_space = 0;
                line[0] = '\0';
                possibly_update_phase = 1;
            }
            continue;
        }

        if (text[char_index] == '"' && !in_comment && !in_multi_comment)
            in_string = !in_string;

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
                    char *tmp = realloc(line, current_size);
                    if (tmp) line = tmp;
                }
                line[used_space++] = '/';
                line[used_space] = '\0';
            }
            entering_comment = 0;
            if (in_comment || in_multi_comment) continue;
        }

        if (text[char_index] == '{' && !in_string && phase == 2) {
            int is_ctrl = is_control_block(line);
            if (control_top < 255) control_stack[control_top++] = is_ctrl;
            brace_depth++;
            if (used_space + 2 >= current_size) {
                current_size *= 2;
                char *tmp = realloc(line, current_size);
                if (tmp) line = tmp;
            }
            line[used_space++] = '{';
            line[used_space] = '\0';
            continue;
        }

        if (text[char_index] == '}' && !in_string && phase == 2) {
            if (brace_depth > 0) {
                brace_depth--;
                if (control_top > 0) control_top--;

                if (used_space + 2 >= current_size) {
                    current_size *= 2;
                    char *tmp = realloc(line, current_size);
                    if (tmp) line = tmp;
                }
                line[used_space++] = '}';
                line[used_space] = '\0';

                if (brace_depth == 0 && used_space > 1) {
                    int lookahead = char_index + 1;
                    while (lookahead < chars && (text[lookahead] == ' ' || text[lookahead] == '\t' || text[lookahead] == '\n' || text[lookahead] == '\r')) {
                        if (text[lookahead] == '\n') line_count++;
                        lookahead++;
                    }
                    if (lookahead < chars && strncmp(&text[lookahead], "else", 4) == 0 &&
                        (lookahead + 4 >= chars || (!isalnum((unsigned char)text[lookahead + 4]) && text[lookahead + 4] != '_'))) {
                        char_index = lookahead - 1;
                        continue;
                    }

                    append_to_codeline(&cl, line, line_count);
                    append_to_parsed_program(&pp, cl, phase, line);
                    reset_cl(&cl);
                    used_space = 0;
                    line[0] = '\0';
                }
            }
            continue;
        }

        if (text[char_index] == ';') {
            if (used_space + 2 >= current_size) {
                current_size *= 2;
                char *tmp = realloc(line, current_size);
                if (tmp) line = tmp;
            }
            line[used_space++] = ';';
            line[used_space] = '\0';

            int inside_control = 0;
            for (int k = 0; k < control_top; k++) {
                if (control_stack[k]) { inside_control = 1; break; }
            }

            if (phase < 2 || !inside_control) {
                append_to_codeline(&cl, line, line_count);
                append_to_parsed_program(&pp, cl, phase, line);
                reset_cl(&cl);
                used_space = 0;
                line[0] = '\0';
                if (phase < 2) possibly_update_phase = 1;
            }
            continue;
        }

        if (used_space + 2 >= current_size) {
            current_size *= 2;
            char *tmp = realloc(line, current_size);
            if (tmp) line = tmp;
        }
        line[used_space++] = text[char_index];
        line[used_space] = '\0';
    }

    free(line);
    return pp;
}

ParsedGlobal parseGlobal(ParsedProgram p) {
    ParsedGlobal pg = {NULL, NULL, 0, 0};
    if (!p.variables || p.v_count == 0) return pg;

    int var_alloc  = 4;
    int type_alloc = 4;
    pg.variable_lines = malloc(sizeof(ParsedCodeLine) * var_alloc);
    pg.typedefs       = malloc(sizeof(ParsedCodeLine) * type_alloc);

    for (int i = 0; i < p.v_count; i++) {
        ParsedCodeLine current = p.variables[i];
        if (current.codeLine.count == 0 || !current.codeLine.lines[0]) continue;

        if (strstr(current.codeLine.lines[0], "typedef") != NULL) {
            if (pg.typedefs_count >= type_alloc) {
                type_alloc *= 2;
                ParsedCodeLine *tmp = realloc(pg.typedefs, sizeof(ParsedCodeLine) * type_alloc);
                if (tmp) pg.typedefs = tmp;
            }
            pg.typedefs[pg.typedefs_count++] = current;
        } else {
            if (pg.variables_count >= var_alloc) {
                var_alloc *= 2;
                ParsedCodeLine *tmp = realloc(pg.variable_lines, sizeof(ParsedCodeLine) * var_alloc);
                if (tmp) pg.variable_lines = tmp;
            }
            pg.variable_lines[pg.variables_count++] = current;
        }
    }

    free_unused((void **)&pg.typedefs,       pg.typedefs_count  * sizeof(ParsedCodeLine));
    free_unused((void **)&pg.variable_lines, pg.variables_count * sizeof(ParsedCodeLine));
    return pg;
}

ParsedMain parseMainProgram(ParsedProgram pp) {
    ParsedMain pm = {NULL, 0, NULL, 0};
    if (!pp.main_body || pp.m_count == 0) return pm;

    int var_alloc  = 4;
    int inst_alloc = 4;
    pm.variable_lines = malloc(sizeof(ParsedCodeLine) * var_alloc);
    pm.instructions   = malloc(sizeof(ParsedCodeLine) * inst_alloc);

    for (int i = 0; i < pp.m_count; i++) {
        ParsedCodeLine current_cl = pp.main_body[i];

        if (is_variable(current_cl.codeLine)) {
            if (pm.variables_count >= var_alloc) {
                var_alloc *= 2;
                ParsedCodeLine *tmp = realloc(pm.variable_lines, sizeof(ParsedCodeLine) * var_alloc);
                if (tmp) pm.variable_lines = tmp;
            }
            pm.variable_lines[pm.variables_count++] = current_cl;
        } else {
            if (pm.instructions_count >= inst_alloc) {
                inst_alloc *= 2;
                ParsedCodeLine *tmp = realloc(pm.instructions, sizeof(ParsedCodeLine) * inst_alloc);
                if (tmp) pm.instructions = tmp;
            }
            pm.instructions[pm.instructions_count++] = current_cl;
        }
    }

    free_unused((void **)&pm.variable_lines, pm.variables_count * sizeof(ParsedCodeLine));
    free_unused((void **)&pm.instructions,   pm.instructions_count * sizeof(ParsedCodeLine));
    return pm;
}

ParsedHeaders parseHeaders(ParsedProgram *program) {
    ParsedHeaders result = {NULL, 0};
    if (!program || program->h_count == 0) return result;

    result.count   = program->h_count;
    result.headers = malloc(result.count * sizeof(char *));
    if (!result.headers) { result.count = 0; return result; }

    for (int i = 0; i < program->h_count; i++) {
        const char *current_line = program->headers[i].formattedCodeLine;
        if (!current_line) { result.headers[i] = NULL; continue; }

        const char *start = strpbrk(current_line, "<\"");
        const char *end   = NULL;
        if (start) {
            char target = (*start == '<') ? '>' : '"';
            end = strchr(start + 1, target);
        }

        if (start && end) {
            size_t len = end - start - 1;
            result.headers[i] = malloc(len + 1);
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

void free_parsed_parts(ParsedGlobal *pg, ParsedMain *pm) {
    if (pg) {
        free(pg->variable_lines);
        free(pg->typedefs);
    }
    if (pm) {
        free(pm->variable_lines);
        free(pm->instructions);
    }
}

void free_headers(ParsedHeaders *ph) {
    if (!ph || !ph->headers) return;
    for (int i = 0; i < ph->count; i++)
        if (ph->headers[i]) free(ph->headers[i]);
    free(ph->headers);
}