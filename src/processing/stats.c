

#include "stats.h"

#include "parser2.h"

Stats stats_adder(Stats stat1, Stats *stat2) {
    // somma due stats. potrebbe essere comodo passare un array di stats e il count. da vedere

    if (stat2) {
        // Faccio realloc della size
        Error* etmp = realloc(stat1.errors, stat1.size_of_errors +  stat2->size_of_errors);
        if (etmp) {
            stat1.size_of_errors += stat2->size_of_errors;
            stat1.errors = etmp;
            // La realloc ha avuto successo: posso aggiungere i nuovi errori iterando sull'attributo
            int indiceIniziale = stat1.error_counter; // Si parte dal prossimo elemento vuoto di stat1
            for (int i = 0; i < stat2->error_counter; i++) {
                stat1.errors[indiceIniziale] = stat2->errors[i];
                indiceIniziale++;
            }
            // Si aggiornano le prime stats
            stat1.error_counter +=stat2->error_counter;
            stat1.illegal_names_counter += stat2->illegal_names_counter;
            stat1.wrong_type_counter += stat2->wrong_type_counter;
            stat1.variable_counter += stat2->variable_counter;

            // Facciamo lo stesso con le variabili non utilizzate
            Variable * vtmp = realloc(stat1.unused_variables, stat1.size_of_unused_variables + stat2->size_of_unused_variables);
            if (vtmp) {
                stat1.size_of_unused_variables += stat2->size_of_unused_variables;
                stat1.unused_variables = vtmp;
                int indiceErrore = stat1.unused_variable_counter;
                for (int i = 0; i < stat2->unused_variable_counter; i++) {
                    stat1.unused_variables[indiceErrore] = stat2->unused_variables[i];
                    indiceErrore++;
                }

                // Abbiamo completato: aggiorniamo anche le variabili non utilizzate
                stat1.unused_variable_counter += stat2->unused_variable_counter;
            }

        }
    }
    return stat1;
}

Stats *process_headers(ParsedHeaders ph) {
    //restituisce un array di stats dato un ParsedHeaders. l'idea è che non li carichi tutti insieme in memoria
    //ma che li processi uno alla volta, liberando la ram piano piano, accumulando stats
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

void manage_variables(Variable **variables, int *current_idx, int *allocated_mem, int *currently_needing, ParsedCodeLine *codelines, int count) {
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

// estrae tutti gli identificatori da una stringa e marca come usate le variabili trovate
static void mark_used_in_expr(const char *expr, Variable *vars, int var_count) {
    if (!expr) return;
    char *copy = strdup(expr);
    if (!copy) return;
    // tokenizza su tutto ciò che non è un identificatore
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

// dato "int x = expr" o "x = expr", ritorna un puntatore all'inizio di expr (dopo il primo =)
// ritorna NULL se non c'è assegnamento
static const char *rhs_of_assignment(const char *line) {
    const char *eq = strchr(line, '=');
    if (!eq) return NULL;
    // evita ==
    if (*(eq + 1) == '=') return NULL;
    // evita !=, <=, >=
    if (eq > line && (*(eq - 1) == '!' || *(eq - 1) == '<' || *(eq - 1) == '>')) return NULL;
    return eq + 1;
}

// estrae il nome della variabile a sinistra di un assegnamento (es. "x" da "x = 5" o "int x = 5")
static void lhs_name(const char *line, char *dest, int dest_size) {
    dest[0] = '\0';
    const char *eq = strchr(line, '=');
    if (!eq) return;
    if (*(eq + 1) == '=') return;
    if (eq > line && (*(eq - 1) == '!' || *(eq - 1) == '<' || *(eq - 1) == '>')) return;

    // copia la parte sinistra e prendi l'ultimo token (il nome)
    int len = (int)(eq - line);
    char *left = malloc(len + 1);
    if (!left) return;
    strncpy(left, line, len);
    left[len] = '\0';

    // l'ultimo token alfanumerico è il nome della variabile
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

    // prendo tutte le variabili utiizzate in maniera normale
    for (int i = 0; i < pm.instructions_count; i++) {
        const char *fmt = pm.instructions[i].formattedCodeLine;
        if (!fmt) continue;
        mark_used_in_expr(fmt, vars, var_count);
    }

    // controlla le variabili che sono dipendenti da altre variabili
    //ad esempio int x = 0; int y = x+ 1; allora x se non è utilizzata altrove dipende da y
    // deps[i] = array di indici di variabili da cui la variabile i dipende
    // se una variabile j nel rhs di "int x = f(j)" è usata, allora x è usata
    int **deps      = calloc(var_count, sizeof(int *));
    int  *deps_count = calloc(var_count, sizeof(int));
    if (!deps || !deps_count) { free(deps); free(deps_count); return; }

    for (int i = 0; i < pm.variables_count; i++) {
        const char *fmt = pm.variable_lines[i].formattedCodeLine;
        if (!fmt) continue;

        const char *rhs = rhs_of_assignment(fmt);
        char lhs[256] = "";
        lhs_name(fmt, lhs, sizeof(lhs));
        if (!lhs[0]) continue;

        int lhs_idx = find_variable_idx(vars, var_count, lhs);
        if (lhs_idx == -1) continue;

        if (!rhs) continue;

        // trova tutte le variabili nel rhs e registra la dipendenza
        char *copy = strdup(rhs);
        if (!copy) continue;
        char *tok = strtok(copy, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
        for (int i = 0; i < var_count; i++)
            printf("vars[%d].name = '%s'\n", i, vars[i].name);
            fflush(stdout);
        while (tok) {
            char clean[256];
            extract_pure_identifier(tok, clean);
            printf("token pulito: '%s'\n", clean);
            fflush(stdout);
            int idx = find_variable_idx(vars, var_count, clean);
            printf("idx per '%s': %d\n", clean, idx);
            fflush(stdout);
            if (clean[0]) {
                int dep_idx = find_variable_idx(vars, var_count, clean);
                if (dep_idx != -1 && dep_idx != lhs_idx) {
                    deps[lhs_idx] = realloc(deps[lhs_idx], (deps_count[lhs_idx] + 1) * sizeof(int));
                    if (deps[lhs_idx])
                        deps[lhs_idx][deps_count[lhs_idx]++] = dep_idx;
                }
            }
            tok = strtok(NULL, " \t\n\r,;(){}[]+-*/%=!<>&|^~?:");
        }
        free(copy);
    }

    //  passa l'informazione indietro
    // se x è usata e x dipende da y, allora y è usata
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < var_count; i++) {
            if (!vars[i].used) continue;
            for (int d = 0; d < deps_count[i]; d++) {
                int dep = deps[i][d];
                if (!vars[dep].used) {
                    vars[dep].used = 1;
                    changed = 1;
                }
            }
        }
    }

    for (int i = 0; i < var_count; i++) {
        free(deps[i]);
    }
    free(deps);
    free(deps_count);

    // --- pass 4: assegna errore alle variabili non usate ---
    for (int i = 0; i < var_count; i++) {
        if (vars[i].used == 0 && vars[i].errors_count == 0) {
            vars[i].errors = malloc(sizeof(VariableError));
            if (!vars[i].errors) continue;
            vars[i].errors[0] = VARIABLE_UNUSED_ERROR;
            vars[i].errors_count = 1;
        }
    }
}

Stats stats_calculator(ParsedProgram pp) {
    Stats stats = {0, 0, 0, 0, 0, NULL, 0, NULL, 0};

    ParsedHeaders ph = parseHeaders(&pp);
    Stats *other_stats = (ph.count != 0) ? process_headers(ph) : NULL;

    Variable *variables = malloc(sizeof(Variable));
    if (!variables) return stats;
    int allocated_mem     = sizeof(Variable);
    int currently_needing = 0;
    int current_idx       = 0;

    ParsedGlobal pg = parseGlobal(pp);
    for (int i = 0; i < pg.typedefs_count; i++) {
        char *tname = getStructType(pg.typedefs[i].codeLine);
        if (tname) { add_to_dict(&dict_types, tname); free(tname); }
    }

    manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing, pg.variable_lines, pg.variables_count);

    ParsedMain pm = parseMainProgram(pp);

    manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing, pm.variable_lines, pm.variables_count);

    stats.variable_counter = current_idx;
    printf("[DEBUG vars]\n");
    for (int i = 0; i < current_idx; i++)
        printf("  [%d] name='%s' type='%s'\n", i, variables[i].name, variables[i].type);
    printf("[DEBUG pm.instructions]\n");
    for (int i = 0; i < pm.instructions_count; i++)
        printf("  [%d] '%s'\n", i, pm.instructions[i].formattedCodeLine);
    printf("[DEBUG pm.variable_lines]\n");
    for (int i = 0; i < pm.variables_count; i++)
        printf("  [%d] '%s'\n", i, pm.variable_lines[i].formattedCodeLine);
    check_variable_usage(variables, current_idx, pm);


    for (int i = 0; i < current_idx; i++) {
        Variable *v = &variables[i];
        ParsedCodeLine readable = parseCodeLine(v->line);
        if (readable.formattedCodeLine) {
            clean_newline(readable.formattedCodeLine);
            free(readable.formattedCodeLine);
        }

        for (int e = 0; e < v->errors_count; e++) {
            VariableError e_type = v->errors[e];
            stats.error_counter++;
            if (e_type == VARIABLE_NAME_ERROR) stats.illegal_names_counter++;
            if (e_type == VARIABLE_TYPE_ERROR) stats.wrong_type_counter++;

            if (e_type == VARIABLE_UNUSED_ERROR) {
                stats.unused_variable_counter++;
                Variable *tmp = realloc(stats.unused_variables, sizeof(Variable) * stats.unused_variable_counter);
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
   // Stats final = stats_adder(stats, other_stats);
    free(other_stats);
    free_variable_array(variables, current_idx);
    return stats;
}

void free_variable_errors(Variable *v) {
    if (!v) return;
    if (v->errors) { free(v->errors); v->errors = NULL; }
}

void free_calculator_locals(Variable *variables, int current_idx, ParsedHeaders *ph, ParsedGlobal *pg, ParsedMain *pm) {
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

void free_stats(Stats *s) {
    if (!s) return;
    if (s->errors) {
        for (int i = 0; i < s->error_counter; i++) {
            if (s->errors[i].filename) free(s->errors[i].filename);
        }
        free(s->errors);
    }
    if (s->unused_variables) {
        for (int i = 0; i < s->unused_variable_counter; i++) {
            if (s->unused_variables[i].name)   free(s->unused_variables[i].name);
            if (s->unused_variables[i].type)   free(s->unused_variables[i].type);
            if (s->unused_variables[i].errors) free(s->unused_variables[i].errors);
        }
        free(s->unused_variables);
    }
}