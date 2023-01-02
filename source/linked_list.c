#include "../headers/linked_list.h"

int compare_goods(goods *a, goods *b) {
    return a->id == b->id && a->tons == b->tons && a->lifespan == b->lifespan;
}

struct node* ll_add(struct node *head, goods to_add) {
    struct node *new_node = (struct node*) malloc(sizeof(new_node));
    struct node *cur;

    new_node->element = malloc(sizeof(to_add));
    new_node->element->id = to_add.id;
    new_node->element->tons = to_add.tons;
    new_node->element->lifespan = to_add.lifespan;

    if(head == NULL) {
        new_node->next = NULL;
        return new_node;
    }

    cur = head;

    while (cur->next && cur->element->lifespan < to_add.lifespan) {
        cur = cur->next;
    }
    new_node->next = cur->next;
    cur->next = new_node;
    return head;
}

void ll_print(struct node *head) {
    struct node *cur;

    if(!head) return;

    cur = head;

    while(cur->next) {
        printf("[%d, %d, %d] -> ", cur->element->id, cur->element->tons, cur->element->lifespan);
        cur = cur->next;
    }

    printf("[%d, %d, %d] -> ", cur->element->id, cur->element->tons, cur->element->lifespan);
}

struct node* ll_remove(struct node *head, goods *to_remove) {
    struct node *cur, *pre;

    if(head == NULL) return head;

    cur = head->next;

    if(compare_goods(head->element, to_remove)) {
        free(head->element);
        free(head);
        return cur;
    }
    pre = head;

    while (cur) {
        if(compare_goods(cur->element, to_remove)) {
            pre->next = cur->next;
            free(cur->element);
            free(cur);
            break;
        }
        cur = cur->next;
    }
    return head;
}
