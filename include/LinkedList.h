//
// Created by potta on 26/04/2026.
//

#ifndef SO2_LINKEDLIST_H
#define SO2_LINKEDLIST_H
#include <stddef.h>

typedef struct Node{
    void *data;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t data_size;
}LinkedList;

void pushBack(LinkedList *list, void *newData);

void initList(LinkedList *list, size_t size);


#endif //SO2_LINKEDLIST_H