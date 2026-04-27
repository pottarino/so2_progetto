//
// Created by Showmae on 11-Apr-26.
//

#ifndef SO2_STATS_H
#define SO2_STATS_H
#include "errors.h"
#include "hashtable.h"
#include "variable_recognizer.h"

/** Controllare singolarmente le variabili è un modo estremamente inefficiente di operare.
 *  L'obiettivo di questa parte è quindi quello di avere un modo di scorrere una sola volta
 *  un testo per rilevare:
 *
 * Numero di variabili non utilizzate
 * Numero di nomi di variabili non corretti
 * Numero di tipi di dato non corretti
 * Numero totale di variabili controllate
 * Numero totale di errori rilevati
 * Per ogni errore rilevato, il numero di riga nel file ( E NOME DEL FILE??)
 * Per ogni variabile non utilizzata, il nome della variabile
 *
 * Appoggio su strutture dati pre-esistenti e definite finora:
 *  Si lavora su ParsedMain e ParsedGlobal quindi sappiamo già quali CodeLine riguardano le variabili e quali le istruzioni
 *  Possiamo allora lavorare su due funzioni separate: una che lavori sulle CodeLines riguardanti le variabili, ed una
 *  che lavori sulle CodeLines riguardanti le istruzioni
 *
 *  Vanno definite le seguenti strutture dati:
 *  1) Una ParsedCodeLine, ovvero una CodeLine contenente l'istruzione formattata FATTA
 *  2) una Variable, ottenuta da una ParsedCodeLine, che ha come attributi: nome, tipo di dato e valore FATTA
 *  3) le variables andranno messe in una mappa che consente di collegare ciascun nome di variabile ad una tupla contente quattro dati:
 *      declaredName, declarationLineNumber, usedBit, e deletedBit ( per free malloc di declaredName) FATTA
 *
 *  Vanno definite le seguenti funzioni di appoggio:
 *  1)  funzione per ottenere la ParsedCodeLine FATTA ( occhio che parse iniziali eliminano ";")
 *  2)  funzione che, data in input una ParsedCodeLine riguardante una variabile ed una mappa, può rilevare errori relativi,
 *      generare o aggiornare la Variable, e fare lo store nella mappa della stessa; A tal proposito occorre riadattare
 *  3) funzione per controllare la correttezza di istruzioni?
 *
 *
 *  La funzione finale dovrà scorrere prima ParsedGlobal, poi ParsedMain:
 *
 *  //  Da definire
 *      Per ParsedGlobal:
 *          1) variable lines:
 *          2) type defs:
 *
 *      Per ParsedMain:
 *          1) variable lines:
 *          2) instructions:
 */

typedef struct{
    int variable_counter;
    int error_counter;
    int unused_variable_counter;
    int illegal_names_counter;
    int wrong_type_counter;
    Error * errors;
    int size_of_errors; // Manteniamo esplicitamente per non dover effettuare ricalcoli nelle adds
    Variable * unused_variables;
    int size_of_unused_variables; // Come sopra, serve per poter fare aggiunte senza ricalcolare le alloc
} Stats;


Stats stats_calculator(ParsedProgram pp);
void analyze_program(char* filename);
#endif //SO2_STATS_H
