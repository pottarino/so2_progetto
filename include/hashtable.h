//
// Created by Showmae on 12-Apr-26.
//

#ifndef SO2_HASHTABLE_H
#define SO2_HASHTABLE_H

/**
 * Variable è una struttura dati che rappresenta una variabile dichiarata: ci interessano il nome, dove viene dichiarata
 * e se viene usata. Lo scopo è quello di poter raccogliere le variabili per controllare in seguito se sono
 * state utilizzate confrontato lo usedBit. Il resto degli errori relativi dichiarazioni e variabili non farà
 * uso di questa struttura dati.
 * La seconda struttura dati qui definita è la HashTable, che è adattata ad ospitare le Variables.
 *
 * NOTA: le Variables vanno inserite nella mappa corrispondente e aggiornate durante
 * l'analisi sequenziale delle ParsedCodeLines. Per generare il File di stats. Al termine, la funzione per generare i
 * files dovrà analizzare la mappa in cui abbiamo inserito queste Variables.
 */


#define TABLE_SIZE 151 // primo > 100 elementi, dovrebbe essere adatto alle variabili di un programma
// se troppo piccola cambiamo a 769 che è primo > 500 elementi




unsigned int hash(const char* nomeVariabile);


typedef struct Element{
    char* name;
    struct Element *next;
}Element;

// Creazione
typedef struct HashTable{
    Element* table[TABLE_SIZE];
} HashTable;


int hashTableInsert(HashTable *ht, const char* name);
int hashTableLookup(HashTable *ht, const char *name);
void hashTableRemove(HashTable *ht, const char *name);

#endif //SO2_HASHTABLE_H
