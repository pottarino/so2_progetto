#include "hashtable.h"
// Funzione di hashing ottimizzata per le stringhe
unsigned int hash(const char *nomeVariabile) {
    int lunghezzaNome = strlen(nomeVariabile);
    unsigned int hashCode = 5381; // dovrebbe essere il migliore per le stringhe
    for (int i = 0; i < lunghezzaNome; i++) {
        hashCode += nomeVariabile[i];
        hashCode *= 37;
    }
    return hashCode % TABLE_SIZE;
}


// Inizializzazione settando i puntatori a NULL
void initHashTable(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        ht->table[i] = NULL;
    }
}

// Funzione per inserire ( si inseriscono puntatori)
bool hashTableInsert(HashTable *ht, Variable *var) {
    if (var == NULL) return false;
    int index = hash(var->declaredName);
    for (int i = 0; i < TABLE_SIZE; i++) {
        // Le collisioni vengono gestitite con una ricerca sequenziale del prossimo spazio libero
        // lo spazio può essere null o riutilizzato da una Variable con deletedBit ad 1
        int indice = (index + i) % TABLE_SIZE;
        if (ht->table[indice] == NULL || ht->table[indice]->deletedBit == 1) {
            ht->table[indice] = var;
            return true;
        }
    }
    return false;
}

// Funzione per cercare una variabile (NULL se diversa o se non presente)
Variable *hashTableLookup(HashTable *ht, char *variableName) {
    int index = hash(variableName);
    for (int i = 0; i < TABLE_SIZE; i++) {
        // Se non lo trova subito scorre con ricerca sequenziale
        int indice = (index + i) % TABLE_SIZE;
        if (ht->table[indice] == NULL) return NULL;
        if (ht->table[indice]->deletedBit == 1) continue;
        if (strcmp(variableName, ht->table[indice]->declaredName) == 0) {
            return ht->table[indice];
        }
    }
    return NULL;
}

// Funzione per rimuovere un elemento dalla mappa
Variable *hashTableRemove(HashTable *ht, char *variableName) {
    int index = hash(variableName);
    for (int i = 0; i < TABLE_SIZE; i++) {
        // Se non lo trova subito scorre con ricerca sequenziale (linear probing)
        int indice = (index + i) % TABLE_SIZE;
        if (ht->table[indice] == NULL) return NULL;
        if (ht->table[indice]->deletedBit == 1) continue;
        if (strcmp(variableName, ht->table[indice]->declaredName) == 0) {
            // Libero la memoria allocata per la variabile e setto il bit rimosso ad 1
            ht->table[indice]->deletedBit = 1;
            free(ht->table[indice]->declaredName);
            ht->table[indice]->declaredName = NULL;
            return ht->table[indice];
        }
    }
    return NULL;
}
