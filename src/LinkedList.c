//
// Created by potta on 26/04/2026.
//

#include "LinkedList.h"

#include <stdlib.h>

// Inizializzazione
void initList(LinkedList *list, size_t size) {
    list->head = NULL;
    list->tail = NULL;
    list->data_size = size;
}

// Inserimento in coda (Push Back)
void pushBack(LinkedList *list, void *newData) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->data = malloc(list->data_size);
    newNode->next = NULL;

    // Copia i byte del dato nell'area allocata
    for (int i = 0; i < list->data_size; i++)
        *(char *)(newNode->data + i) = *(char *)(newData + i);

    if (list->head == NULL) {
        newNode->prev = NULL;
        list->head = newNode;
        list->tail = newNode;
    } else {
        newNode->prev = list->tail;
        list->tail->next = newNode;
        list->tail = newNode;
    }
}