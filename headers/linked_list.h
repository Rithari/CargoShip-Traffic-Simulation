#ifndef PROGETTOSO_LINKED_LIST_H
#define PROGETTOSO_LINKED_LIST_H

#include "master.h"

struct node{
    goods *element;
    struct node *next;
};

struct node *ll_add(struct node*, void*, size_t);
void ll_print(struct node *head);

#endif /*PROGETTOSO_LINKED_LIST_H*/
