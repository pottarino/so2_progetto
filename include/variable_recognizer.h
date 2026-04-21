#ifndef VARIABLE_RECOGNIZER_H
#define VARIABLE_RECOGNIZER_H
#include "parsers.h"
#include "utility.h"


// Struttura dati per tenere traccia di nome e utilizzo variabili



typedef struct {
    CodeLine line;
    char* type;
    char* name;
    VariableError *errors;
    int errors_count;
    int deletedBit;
    int used;
} Variable;
/**
 * Verifica se una stringa segue le regole degli identificatori C
 * e non sia una parola riservata.
 */
int is_name_valid(const char *name);


/**
 * ritorna il nome del nuovo tipo definito
 */
char* getStructType(CodeLine structDefinition);
/**
 * Analizza la struttura CodeLine alla ricerca di una 
 * dichiarazione di variabile (modificatori + tipo + nome).
 */
int is_variable(CodeLine codeline);
Variable* parse_variable_declaration(CodeLine codeline, int* total_found);
void free_variable_array(Variable *v, int count);
void extract_pure_identifier(const char* src, char* dest);
#endif