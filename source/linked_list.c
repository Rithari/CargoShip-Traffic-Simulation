#include "../headers/linked_list.h"

struct node* add(struct node *head, goods g) {

    struct node *new_node = (struct node*) malloc(sizeof(new_node));
    new_node->element = &g;
    new_node->next = NULL;

    if(head == NULL) {
        return new_node;
    }

    struct node *cur = head;

    while (cur->next) {
        cur = cur->next;
    }

    cur->next = new_node;
    return head;
}

void ll_print(struct node *head) {

    if(!head) return;

    struct node *cur = head;

    while(cur->next) {
        printf("[%d, %d, %d] -> ", cur->element->id, cur->element->tons, cur->element->lifespan);
        cur = cur->next;
    }

    printf("[%d, %d, %d] -> ", cur->element->id, cur->element->tons, cur->element->lifespan);
}
