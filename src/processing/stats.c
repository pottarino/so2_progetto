#include "stats.h"
#include "parser2.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "variable_recognizer.h"

HashTable *headers_table;

Stats stats_adder(Stats stat1, Stats stat2) {
    stat1.illegal_names_counter += stat2.illegal_names_counter;
    stat1.wrong_type_counter    += stat2.wrong_type_counter;
    stat1.variable_counter      += stat2.variable_counter;

    if (stat2.error_counter > 0) {
        Error *etmp = realloc(stat1.errors,
            (stat1.error_counter + stat2.error_counter) * sizeof(Error));
        if (etmp) {
            stat1.errors = etmp;
            for (int i = 0; i < stat2.error_counter; i++) {
                stat1.errors[stat1.error_counter + i] = stat2.errors[i];
                if (stat2.errors[i].filename)
                    stat1.errors[stat1.error_counter + i].filename = strdup(stat2.errors[i].filename);
            }
            stat1.error_counter += stat2.error_counter;
            stat1.size_of_errors = stat1.error_counter * sizeof(Error);
        }
    }

    if (stat2.unused_variable_counter > 0) {
        Variable *vtmp = realloc(stat1.unused_variables,
            (stat1.unused_variable_counter + stat2.unused_variable_counter) * sizeof(Variable));
        if (vtmp) {
            stat1.unused_variables = vtmp;
            for (int i = 0; i < stat2.unused_variable_counter; i++)
                stat1.unused_variables[stat1.unused_variable_counter + i] = stat2.unused_variables[i];
            stat1.unused_variable_counter += stat2.unused_variable_counter;
            stat1.size_of_unused_variables = stat1.unused_variable_counter * sizeof(Variable);
        }
    }
    return stat1;
}

void manage_variables(Variable **variables, int *current_idx, int *allocated_mem,
                      int *currently_needing, ParsedCodeLine *codelines, int count) {
    for (int i = 0; i < count; i++) {
        int total_vars_line = 0;
        Variable *found_vars = parse_variable_declaration(codelines[i].codeLine, &total_vars_line);
        if (!found_vars) continue;

        for (int j = 0; j < total_vars_line; j++) {
            if ((*current_idx + 1) * (int)sizeof(Variable) > *allocated_mem) {
                if (!allocate_more((void **)variables, allocated_mem, sizeof(Variable))) {
                    free_variable_array(found_vars, total_vars_line);
                    return;
                }
            }

            char *new_name = found_vars[j].name;
            int pos = *current_idx;
            while (pos > 0 && strcmp((*variables)[pos - 1].name, new_name) > 0) {
                (*variables)[pos] = (*variables)[pos - 1];
                pos--;
            }
            (*variables)[pos] = found_vars[j];
            (*current_idx)++;
        }
        free(found_vars);
    }
}

// estrae variabili e typedef da un header, senza cercare il main
// ritorna le variabili trovate e aggiunge i typedef a dict_types
static Variable *extract_header_variables(char *filename, int *out_count) {
    *out_count = 0;
    printf("[DEBUG extract_header_variables] elaboro: '%s'\n", filename); fflush(stdout);

    FileRead fr = file_reader(filename);
    if (!fr.text) {
        printf("[DEBUG extract_header_variables] file non trovato: '%s'\n", filename); fflush(stdout);
        return NULL;
    }
    printf("[DEBUG extract_header_variables] letto, lunghezza=%d\n", (int)strlen(fr.text)); fflush(stdout);

    ParsedProgram pp = parse_program(fr.text, filename);
    free(fr.text);

    // processa ricorsivamente gli header inclusi in questo header
    ParsedHeaders ph = parseHeaders(&pp);
    printf("[DEBUG extract_header_variables] sotto-header: %d\n", ph.count); fflush(stdout);
    for (int i = 0; i < ph.count; i++) {
        char *hname = ph.headers[i];
        if (!hname) continue;
        if (hashTableLookup(headers_table, hname)) continue;
        hashTableInsert(headers_table, hname);
        char *fname = replace_extension(hname, "txt");
        if (!fname) continue;
        FILE *chk = fopen(fname, "r");
        if (!chk) { free(fname); continue; }
        fclose(chk);
        int sub_count = 0;
        Variable *sub_vars = extract_header_variables(fname, &sub_count);
        free(fname);
        // sub_vars vengono aggiunte al pool dal chiamante — qui le scartiamo
        // perché il pool viene costruito in stats_calculator
        // ma i typedef sono già stati aggiunti a dict_types dentro la ricorsione
        if (sub_vars) free(sub_vars);
    }
    free_headers(&ph);

    // aggiungi typedef di questo header a dict_types
    ParsedGlobal pg = parseGlobal(pp);
    for (int i = 0; i < pg.typedefs_count; i++) {
        char *tname = getStructType(pg.typedefs[i].codeLine);
        printf("[DEBUG extract_header_variables] typedef: '%s'\n", tname ? tname : "NULL"); fflush(stdout);
        if (tname) { add_to_dict(&dict_types, tname); free(tname); }
    }

    // estrai variabili globali
    Variable *vars = NULL;
    int allocated = sizeof(Variable);
    vars = malloc(allocated);
    int idx = 0;
    int needing = 0;
    manage_variables(&vars, &idx, &allocated, &needing, pg.variable_lines, pg.variables_count);

    printf("[DEBUG extract_header_variables] variabili estratte: %d\n", idx); fflush(stdout);

    free(pg.variable_lines);
    free(pg.typedefs);

    *out_count = idx;
    if (idx == 0) { free(vars); return NULL; }
    return vars;
}

int find_variable_idx(Variable *vars, int count, const char *name) {
    int low = 0, high = count - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(vars[mid].name, name);
        if (cmp == 0) return mid;
        if (cmp < 0) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}


static void mark_used_in_expr(const char *expr, Variable *vars, int var_count) {
    if (!expr) return;
    char *copy = strdup(expr);
    if (!copy) return;
    char *tok = strtok(copy, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
    while (tok) {
        char clean[256];
        extract_pure_identifier(tok, clean);
        if (clean[0]) {
            int idx = find_variable_idx(vars, var_count, clean);
            if (idx != -1) vars[idx].used = 1;
        }
        tok = strtok(NULL, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
    }
    free(copy);
}

static const char *rhs_of_assignment(const char *line) {
    const char *eq = strchr(line, '=');
    if (!eq) return NULL;
    if (*(eq + 1) == '=') return NULL;
    if (eq > line && (*(eq-1) == '!' || *(eq-1) == '<' || *(eq-1) == '>')) return NULL;
    return eq + 1;
}

static void lhs_name(const char *line, char *dest, int dest_size) {
    dest[0] = '\0';
    const char *eq = strchr(line, '=');
    if (!eq) return;
    if (*(eq + 1) == '=') return;
    if (eq > line && (*(eq-1) == '!' || *(eq-1) == '<' || *(eq-1) == '>')) return;

    int len = (int)(eq - line);
    char *left = malloc(len + 1);
    if (!left) return;
    strncpy(left, line, len);
    left[len] = '\0';

    char *last = NULL;
    char *tok = strtok(left, " \t*,");
    while (tok) { last = tok; tok = strtok(NULL, " \t*,"); }
    if (last) {
        char clean[256];
        extract_pure_identifier(last, clean);
        strncpy(dest, clean, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
    free(left);
}

void check_variable_usage(Variable *vars, int var_count, ParsedMain pm) {
    for (int i = 0; i < var_count; i++) vars[i].used = 0;

    for (int i = 0; i < pm.instructions_count; i++) {
        const char *fmt = pm.instructions[i].formattedCodeLine;
        if (!fmt) continue;
        mark_used_in_expr(fmt, vars, var_count);
    }

    int **deps       = calloc(var_count, sizeof(int *));
    int  *deps_count = calloc(var_count, sizeof(int));
    if (!deps || !deps_count) { free(deps); free(deps_count); return; }

    for (int i = 0; i < pm.variables_count; i++) {
        const char *fmt = pm.variable_lines[i].formattedCodeLine;
        if (!fmt) continue;
        const char *rhs = rhs_of_assignment(fmt);
        char lhs[256] = "";
        lhs_name(fmt, lhs, sizeof(lhs));
        if (!lhs[0] || !rhs) continue;
        int lhs_idx = find_variable_idx(vars, var_count, lhs);
        if (lhs_idx == -1) continue;
        char *copy = strdup(rhs);
        if (!copy) continue;
        char *tok = strtok(copy, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
        while (tok) {
            char clean[256];
            extract_pure_identifier(tok, clean);
            if (clean[0]) {
                int dep_idx = find_variable_idx(vars, var_count, clean);
                if (dep_idx != -1 && dep_idx != lhs_idx) {
                    int *tmp = realloc(deps[lhs_idx], (deps_count[lhs_idx] + 1) * sizeof(int));
                    if (tmp) { deps[lhs_idx] = tmp; deps[lhs_idx][deps_count[lhs_idx]++] = dep_idx; }
                }
            }
            tok = strtok(NULL, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
        }
        free(copy);
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < var_count; i++) {
            if (!vars[i].used) continue;
            for (int d = 0; d < deps_count[i]; d++) {
                int dep = deps[i][d];
                if (!vars[dep].used) { vars[dep].used = 1; changed = 1; }
            }
        }
    }

    for (int i = 0; i < var_count; i++) free(deps[i]);
    free(deps);
    free(deps_count);

    for (int i = 0; i < var_count; i++) {
        if (vars[i].used == 0 && vars[i].errors_count == 0) {
            vars[i].errors = malloc(sizeof(VariableError));
            if (!vars[i].errors) continue;
            vars[i].errors[0] = VARIABLE_UNUSED_ERROR;
            vars[i].errors_count = 1;
        }
    }
}

void free_calculator_locals(Variable *variables, int current_idx,
                            ParsedHeaders *ph, ParsedGlobal *pg, ParsedMain *pm) {
    free_variable_array(variables, current_idx);
    if (ph) free_headers(ph);
    if (pg) {
        for (int i = 0; i < pg->variables_count; i++) free_codeline(&pg->variable_lines[i].codeLine);
        for (int i = 0; i < pg->typedefs_count;  i++) free_codeline(&pg->typedefs[i].codeLine);
        free(pg->variable_lines);
        free(pg->typedefs);
    }
    if (pm) {
        for (int i = 0; i < pm->variables_count;    i++) free_codeline(&pm->variable_lines[i].codeLine);
        for (int i = 0; i < pm->instructions_count; i++) free_codeline(&pm->instructions[i].codeLine);
        free(pm->variable_lines);
        free(pm->instructions);
    }
}

Stats stats_calculator(ParsedProgram pp) {
    Stats stats = {0, 0, 0, 0, 0, NULL, 0, NULL, 0};

    printf("[DEBUG stats_calculator] h=%d v=%d m=%d\n", pp.h_count, pp.v_count, pp.m_count); fflush(stdout);

    // pool unico di variabili — header + globali + main
    Variable *variables = malloc(sizeof(Variable));
    if (!variables) return stats;
    int allocated_mem     = sizeof(Variable);
    int currently_needing = 0;
    int current_idx       = 0;

    // 1. processa gli header: typedef → dict_types, variabili → pool
    ParsedHeaders ph = parseHeaders(&pp);
    printf("[DEBUG stats_calculator] processando %d header(s)\n", ph.count); fflush(stdout);
    for (int i = 0; i < ph.count; i++) {
        char *hname = ph.headers[i];
        printf("[DEBUG stats_calculator] header[%d] = '%s'\n", i, hname ? hname : "NULL"); fflush(stdout);
        if (!hname) continue;
        if (hashTableLookup(headers_table, hname)) {
            printf("[DEBUG stats_calculator] header '%s' gia' processato\n", hname); fflush(stdout);
            continue;
        }
        hashTableInsert(headers_table, hname);
        char *fname = replace_extension(hname, "txt");
        if (!fname) continue;
        FILE *chk = fopen(fname, "r");
        if (!chk) {
            printf("[DEBUG stats_calculator] header file '%s' non trovato\n", fname); fflush(stdout);
            free(fname);
            continue;
        }
        fclose(chk);
        int h_count = 0;
        Variable *h_vars = extract_header_variables(fname, &h_count);
        free(fname);
        printf("[DEBUG stats_calculator] variabili da header: %d\n", h_count); fflush(stdout);
        if (h_vars && h_count > 0) {
            for (int j = 0; j < h_count; j++) {
                if ((current_idx + 1) * (int)sizeof(Variable) > allocated_mem)
                    allocate_more((void **)&variables, &allocated_mem, sizeof(Variable));
                char *new_name = h_vars[j].name;
                int pos = current_idx;
                while (pos > 0 && strcmp(variables[pos - 1].name, new_name) > 0) {
                    variables[pos] = variables[pos - 1];
                    pos--;
                }
                variables[pos] = h_vars[j];
                current_idx++;
            }
            free(h_vars);
        }
    }
    free_headers(&ph);

    // 2. typedef del file principale
    ParsedGlobal pg = parseGlobal(pp);
    printf("[DEBUG stats_calculator] typedefs=%d global_vars=%d\n", pg.typedefs_count, pg.variables_count); fflush(stdout);
    for (int i = 0; i < pg.typedefs_count; i++) {
        char *tname = getStructType(pg.typedefs[i].codeLine);
        printf("[DEBUG stats_calculator] typedef: '%s'\n", tname ? tname : "NULL"); fflush(stdout);
        if (tname) { add_to_dict(&dict_types, tname); free(tname); }
    }

    // 3. variabili globali del file principale
    manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing,
                     pg.variable_lines, pg.variables_count);

    // 4. variabili locali del main
    ParsedMain pm = parseMainProgram(pp);
    printf("[DEBUG stats_calculator] pm.variables=%d pm.instructions=%d\n",
           pm.variables_count, pm.instructions_count); fflush(stdout);
    manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing,
                     pm.variable_lines, pm.variables_count);

    stats.variable_counter = current_idx;
    printf("[DEBUG stats_calculator] variabili totali nel pool: %d\n", current_idx); fflush(stdout);
    for (int i = 0; i < current_idx; i++) {
        printf("[DEBUG stats_calculator] var[%d] name='%s' type='%s'\n",
               i, variables[i].name, variables[i].type); fflush(stdout);
    }

    // 5. check utilizzo — unico, sul main del file principale
    check_variable_usage(variables, current_idx, pm);

    // 6. costruisci stats
    for (int i = 0; i < current_idx; i++) {
        Variable *v = &variables[i];
        for (int e = 0; e < v->errors_count; e++) {
            VariableError e_type = v->errors[e];
            stats.error_counter++;
            if (e_type == VARIABLE_NAME_ERROR) stats.illegal_names_counter++;
            if (e_type == VARIABLE_TYPE_ERROR) stats.wrong_type_counter++;

            if (e_type == VARIABLE_UNUSED_ERROR) {
                stats.unused_variable_counter++;
                Variable *tmp = realloc(stats.unused_variables,
                    sizeof(Variable) * stats.unused_variable_counter);
                if (!tmp) { stats.unused_variable_counter--; continue; }
                stats.unused_variables = tmp;
                stats.size_of_unused_variables = sizeof(Variable) * stats.unused_variable_counter;
                stats.unused_variables[stats.unused_variable_counter - 1] = *v;
            }

            Error *etmp = realloc(stats.errors, sizeof(Error) * stats.error_counter);
            if (!etmp) continue;
            stats.errors = etmp;
            stats.size_of_errors = sizeof(Error) * stats.error_counter;
            Error new_error;
            new_error.type     = e_type;
            new_error.line     = (v->line.line_numbers && v->line.count > 0) ? v->line.line_numbers[0] : -1;
            new_error.filename = v->line.filename ? strdup(v->line.filename) : NULL;
            stats.errors[stats.error_counter - 1] = new_error;
        }
    }

    free_calculator_locals(variables, current_idx, NULL, &pg, &pm);
    return stats;
}

void free_variable_errors(Variable *v) {
    if (!v) return;
    if (v->errors) { free(v->errors); v->errors = NULL; }
}

void free_stats(Stats *s) {
    if (!s) return;
    if (s->errors) {
        for (int i = 0; i < s->error_counter; i++)
            if (s->errors[i].filename) free(s->errors[i].filename);
        free(s->errors);
        s->errors = NULL;
    }
    if (s->unused_variables) {
        free(s->unused_variables);
        s->unused_variables = NULL;
    }
}

Stats *test_stats_calculator(char *filename) {
    printf("[DEBUG test_stats_calculator] elaboro: '%s'\n", filename); fflush(stdout);

    FileRead fr = file_reader(filename);
    if (!fr.text) {
        printf("[DEBUG test_stats_calculator] file non trovato: '%s'\n", filename); fflush(stdout);
        return NULL;
    }
    printf("[DEBUG test_stats_calculator] letto, lunghezza=%d\n", (int)strlen(fr.text)); fflush(stdout);

    ParsedProgram pp = parse_program(fr.text, filename);
    free(fr.text);

    Stats *stats_ptr = malloc(sizeof(Stats));
    if (stats_ptr) *stats_ptr = stats_calculator(pp);
    return stats_ptr;
}

void analyze_program(char *filename) {
    printf("[DEBUG analyze_program] avvio: '%s'\n", filename); fflush(stdout);

    headers_table = calloc(1, sizeof(HashTable));
    if (!headers_table) return;

    init_syntax();
    hashTableInsert(headers_table, filename);

    Stats *final_stats = test_stats_calculator(filename);

    if (final_stats) {
        printf("\n--- RISULTATI STATISTICHE FINALI: %s ---\n", filename); fflush(stdout);
        printf("variabili totali:       %d\n", final_stats->variable_counter); fflush(stdout);
        printf("nomi illegali:          %d\n", final_stats->illegal_names_counter); fflush(stdout);
        printf("tipi errati:            %d\n", final_stats->wrong_type_counter); fflush(stdout);
        printf("variabili inutilizzate: %d\n", final_stats->unused_variable_counter); fflush(stdout);
        printf("totale errori:          %d\n", final_stats->error_counter); fflush(stdout);

        printf("\n--- DETTAGLIO ERRORI ---\n"); fflush(stdout);
        for (int i = 0; i < final_stats->error_counter; i++) {
            const char *err_name =
                final_stats->errors[i].type == VARIABLE_NAME_ERROR   ? "NOME_ILLEGALE" :
                final_stats->errors[i].type == VARIABLE_TYPE_ERROR   ? "TIPO_ERRATO"   :
                final_stats->errors[i].type == VARIABLE_UNUSED_ERROR ? "NON_USATA"     : "SCONOSCIUTO";
            printf("[%s] rigo %d nel file %s\n",
                err_name,
                final_stats->errors[i].line,
                final_stats->errors[i].filename ? final_stats->errors[i].filename : "?"); fflush(stdout);
        }

        free_stats(final_stats);
        free(final_stats);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        Element *e = headers_table->table[i];
        while (e) {
            Element *next = e->next;
            free(e->name);
            free(e);
            e = next;
        }
    }
    free(headers_table);
    headers_table = NULL;
    printf("[DEBUG analyze_program] completato\n"); fflush(stdout);
}