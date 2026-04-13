//
// Created by potta on 13/04/2026.
//

#include "parsers.h"
#include "stats.h"

#include <ctype.h>

#include "utility.h"

Stats stats_adder(Stats stat1, Stats * stat2) {
  //da fare
  if (stat2 == NULL){return stat1;}
  Stats stat_other = *stat2;
  //...
}

Stats * process_headers(ParsedHeaders ph) {
  // da fare
}

int find_variable_idx(Variable* vars, int count, const char* name) {
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

void manage_variables(Variable **variables, int *current_idx, int *allocated_mem, int *currently_needing, CodeLine *codelines, int count) {
  int total_vars_line = 0;

  for (int i = 0; i < count; i++) {
    Variable* found_vars = parse_variable_declaration(codelines[i], &total_vars_line);
    if (!found_vars) continue;

    for (int j = 0; j < total_vars_line; j++) {
      *currently_needing += sizeof(Variable);
      if (*currently_needing > *allocated_mem) {
        allocate_more((void**)variables, allocated_mem);
      }

      char* new_name = found_vars[j].name;
      int pos = *current_idx;

      while (pos > 0 && strcmp((*variables)[pos - 1].name, new_name) > 0) {
        (*variables)[pos] = (*variables)[pos - 1];
        pos--;
      }

      // --- DEEP COPY: Rendiamo la variabile indipendente ---
      (*variables)[pos] = found_vars[j]; // Copia base
      (*variables)[pos].name = strdup(found_vars[j].name); // Nuova copia della stringa
      (*variables)[pos].type = strdup(found_vars[j].type); // Nuova copia del tipo

      // Copiamo i line_numbers per evitare il baadf00d
      if (found_vars[j].line.line_numbers != NULL) {
        int n = found_vars[j].line.count;
        (*variables)[pos].line.line_numbers = malloc(sizeof(int) * n);
        if (found_vars[j].line.line_numbers != NULL && n > 0) {
          // 1. ALLOCA nuovo spazio per la destinazione
          (*variables)[pos].line.line_numbers = malloc(sizeof(int) * n);

          if ((*variables)[pos].line.line_numbers != NULL) {
            // 2. COPIA i dati dalla sorgente alla nuova memoria appena allocata
            memcpy((*variables)[pos].line.line_numbers, found_vars[j].line.line_numbers, sizeof(int) * n);
          }
        } else {
          (*variables)[pos].line.line_numbers = NULL;
        }
      }
      (*variables)[pos].line.filename = strdup(found_vars[j].line.filename);

      (*current_idx)++;
    }

    free(found_vars);
  }
}

void extract_pure_identifier(const char* src, char* dest) {
  int i = 0, j = 0;
  // Salta eventuali simboli iniziali (es: *ptr o &var)
  while (src[i] != '\0' && !isalnum((unsigned char)src[i]) && src[i] != '_') i++;

  // Copia finché è un carattere valido per un nome C
  while (src[i] != '\0' && (isalnum((unsigned char)src[i]) || src[i] == '_')) {
    dest[j++] = src[i++];
    if (j >= 255) break;
  }
  dest[j] = '\0';
}

void check_variable_usage(Variable* vars, int var_count, ParsedMain pm) {
  // azzeriamo l'uso (giusto per sicurezza)
  for(int i=0; i<var_count; i++) vars[i].used = 0;

  for (int i = 0; i < pm.instructions_count; i++) {
    ParsedCodeLine pcl = parseCodeLine(pm.instructions[i]);
    char** tokens = split(pcl.formattedCodeLine, " ");
    if (!tokens) { free(pcl.formattedCodeLine); continue; }

    for (int j = 0; tokens[j] != NULL; j++) {
      char clean[256];
      extract_pure_identifier(tokens[j], clean);

      int idx = find_variable_idx(vars, var_count, clean);
      if (idx != -1) {
        vars[idx].used = 1;
      }
    }
    free_split(tokens);
    free(pcl.formattedCodeLine);
  }

  // ora capiamo
  for (int i = 0; i < var_count; i++) {
    if (vars[i].used == 0 && vars[i].errors == NULL) {
      vars[i].errors = malloc(sizeof(VariableError));
      vars[i].errors[0] = VARIABLE_UNUSED_ERROR;
      vars[i].errors_count++;
    }
  }
}

Stats stats_calculator(ParsedProgram pp) {
  Stats stats = {0, 0, 0, 0, 0, NULL, NULL};

  // gestisco gli eaders
  ParsedHeaders ph = parseHeaders(&pp);
  Stats *other_stats = (ph.count != 0) ? process_headers(ph) : NULL;
  // TOD: free_parsed_headers(ph);

  // setuppo la memoria per le variabili
  Variable *variables = malloc(sizeof(Variable));
  int allocated_mem = sizeof(Variable);
  int currently_needing = 0;
  int current_idx = 0;

  // gestisco il global
  ParsedGlobal pg = parseGlobal(pp);
  // aggiungi nuovi tipi al dizionario
  for (int i = 0; i < pg.typedefs_count; i++) {
    add_to_dict(&dict_keywords, getStructType(pg.typedefs[i]));
  }

  // gestisci variabili globali
  manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing, pg.variable_lines, pg.variables_count);

  // 4. main
  ParsedMain pm = parseMainProgram(pp);
  // gestisci variabili locali (main)
  manage_variables(&variables, &current_idx, &allocated_mem, &currently_needing, pm.variable_lines, pm.variables_count);

  stats.variable_counter = current_idx;

  check_variable_usage(variables, current_idx, pm);

  for (int i = 0; i < current_idx; i++) {
    //ora per ogni variabile la ricontrolliamo
    Variable* v = &variables[i];

    // se la variabile ha errori aggiorniamo i counter
    for (int e = 0; e < v->errors_count; e++) {
        VariableError e_type = v->errors[e];

        stats.error_counter++;
        if (e_type == VARIABLE_NAME_ERROR) stats.illegal_names_counter++;
        if (e_type == VARIABLE_TYPE_ERROR) stats.wrong_type_counter++;

        if (e_type == VARIABLE_UNUSED_ERROR) {
          stats.unused_variable_counter++;

          // se è unused
          stats.unused_variables = realloc(stats.unused_variables, sizeof(Variable) * stats.unused_variable_counter);
          stats.unused_variables[stats.unused_variable_counter - 1] = *v;
        }

        // 3. Creiamo l'oggetto Error da aggiungere alla lista generale degli errori
        stats.errors = realloc(stats.errors, sizeof(Error) * stats.error_counter);
        Error new_error;
        new_error.type = e_type;
        new_error.line = v->line.line_numbers[0]; // Se crasha qui, line_numbers era NULL

        new_error.filename = strdup(v->line.filename);

        stats.errors[stats.error_counter - 1] = new_error;
      }
    }
  // return stats_adder(stats, other_stats);
  return stats;
  }

// libera un singolo errore o array di errori dentro la variabile
void free_variable_errors(Variable* v) {
    if (v->errors) {
        free(v->errors);
        v->errors = NULL;
    }
}

// libera tutto il contenuto di un array di variabili (nomi, tipi, etc)
void free_variable_array(Variable* vars, int count) {
    if (!vars) return;
    for (int i = 0; i < count; i++) {
        if (vars[i].name) free(vars[i].name);
        if (vars[i].type) free(vars[i].type);
        // la CodeLine dentro la Variable viene liberata altrove o qui se è l'unica copia
        free_variable_errors(&vars[i]);
    }
    free(vars);
}

// libera la memoria locale usata dentro stats_calculator
void free_calculator_locals(Variable* variables, int current_idx, ParsedHeaders* ph, ParsedGlobal* pg, ParsedMain* pm) {
    // 1. libera le variabili temporanee create durante il calcolo
    // occhio: se le passi alle stats come puntatori, non liberarle qui!
    // se hai fatto copie (come con *v), allora libera pure.
    free_variable_array(variables, current_idx);

    // 2. libera i risultati del parsing
    if (ph) free_headers(ph);
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

// libera la struttura Stats finale
void free_stats(Stats* s) {
    if (!s) return;

    // libera l'array degli errori
    if (s->errors) {
        for (int i = 0; i < s->error_counter; i++) {
            if (s->errors[i].filename) free(s->errors[i].filename);
        }
        free(s->errors);
    }

    // libera le variabili unused (che sono copie)
    if (s->unused_variables) {
        for (int i = 0; i < s->unused_variable_counter; i++) {
            if (s->unused_variables[i].name) free(s->unused_variables[i].name);
            if (s->unused_variables[i].type) free(s->unused_variables[i].type);
            if (s->unused_variables[i].errors) free(s->unused_variables[i].errors);
            // se CodeLine ha roba mallocata, libera anche quella
        }
        free(s->unused_variables);
    }
}