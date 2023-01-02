#ifndef PROGETTOSO_LINKED_LIST_H
#define PROGETTOSO_LINKED_LIST_H

#include "master.h"

struct node{
    goods *element;
    struct node *next;
};

struct node *ll_add(struct node*, goods to_add);
void ll_print(struct node *head);
struct node* ll_remove(struct node *head, goods *to_remove);

#endif /*PROGETTOSO_LINKED_LIST_H*/
