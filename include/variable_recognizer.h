#ifndef VARIABLE_RECOGNIZER_H
#define VARIABLE_RECOGNIZER_H
#include "parsers.h"
#include "utility.h"


/**
 * Verifica se una stringa segue le regole degli identificatori C
 * e non sia una parola riservata.
 */
int is_name_valid(const char *name);

/**
 * Analizza la struttura CodeLine alla ricerca di una 
 * dichiarazione di variabile (modificatori + tipo + nome).
 */
int is_variable(CodeLine codeline);



#endif