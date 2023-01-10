#ifndef PROGETTOSO_LINKED_LIST_H
#define PROGETTOSO_LINKED_LIST_H

#include "master.h"

struct node{
    goods *element;
    struct node *next;
};

struct node *ll_add(struct node*, goods *to_add);
void ll_print(struct node *head);
struct node* ll_remove_index(struct node *head, int index);
struct node* ll_remove_by_id(struct node *head, int id);
struct node *generate_ship_goods_list(struct node *head, struct node *sublist, int* how_many, int* arr, int size);
struct node* ll_pop(struct node *head);
void ll_free(struct node *head);

#endif /*PROGETTOSO_LINKED_LIST_H*/
