#include "hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int hash(const char *name) {
    unsigned int hashCode = 5381;
    if (name == NULL) return -1;
    while (*name) {
        hashCode = ((hashCode << 5) + hashCode) + (unsigned char)(*name++);
    }
    return hashCode % TABLE_SIZE;
}


int hashTableInsert(HashTable *ht, const char* name) {
    unsigned int index = hash(name);
    if (index == -1) return 0;
    // Controllo se esiste già
    Element *curr = ht->table[index];
    while (curr) {
        if (strcmp(curr->name, name) == 0) return 0;
        curr = curr->next;
    }

    // Inserimento in testa (più veloce)
    Element *new_el = malloc(sizeof(Element));
    new_el->name = strdup(name); // Copia sicura della stringa
    new_el->next = ht->table[index];
    ht->table[index] = new_el;

    return 1;
}

int hashTableLookup(HashTable *ht, const char *name) {
    unsigned int index = hash(name);
    if (index == -1) return -1;
    Element *curr = ht->table[index];
    while (curr) {
        if (strcmp(curr->name, name) == 0) return 1;
        curr = curr->next;
    }
    return 0;
}

void hashTableRemove(HashTable *ht, const char *name) {
    unsigned int index = hash(name);
    Element *curr = ht->table[index];
    Element *prev = NULL;

    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            if (prev) prev->next = curr->next;
            else ht->table[index] = curr->next;

            free(curr->name);
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}