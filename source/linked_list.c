#include "../headers/linked_list.h"

int compare_goods(goods *a, goods *b) {
    return a->id == b->id && a->quantity == b->quantity && a->lifespan == b->lifespan;
}

struct node* ll_add(struct node *head, goods *to_add) {
    struct node *new_node;
    struct node *cur;

    if(head == NULL) {
        new_node = (struct node*) malloc(sizeof(struct node));
        new_node->element = malloc(sizeof(*to_add));
        new_node->element->id = to_add->id;
        new_node->element->quantity = to_add->quantity;
        new_node->element->lifespan = to_add->lifespan;
        new_node->next = NULL;
        return new_node;
    }

    cur = head;

    if (cur->element->lifespan == to_add->lifespan && cur->element->id == to_add->id) {
        cur->element->quantity += to_add->quantity;
        return head;
    }

    while (cur->next && cur->element->lifespan <= to_add->lifespan) {
        if (cur->element->lifespan == to_add->lifespan && cur->element->id == to_add->id) {
            cur->element->quantity += to_add->quantity;
            return head;
        }
        cur = cur->next;
    }

    new_node = (struct node*) malloc(sizeof(struct node));
    new_node->element = (goods*) malloc(sizeof(goods));
    new_node->element->id = to_add->id;
    new_node->element->quantity = to_add->quantity;
    new_node->element->lifespan = to_add->lifespan;
    new_node->next = cur->next;
    cur->next = new_node;
    return head;
}

void ll_print(struct node *head) {
    struct node *cur;

    if(!head) {
        printf("[0/0/0]\n");
        return;
    }

    cur = head;

    while(cur->next) {
        printf("[%d, %d, %d] -> ", cur->element->id, cur->element->quantity, cur->element->lifespan);
        cur = cur->next;
    }

    printf("[%d, %d, %d].\n", cur->element->id, cur->element->quantity, cur->element->lifespan);
}


struct node* ll_remove_by_id(struct node *head, int id) {
    struct node *cur, *pre;

    if(head == NULL) return head;

    cur = head->next;

    if(head->element->id == id) {
        free(head->element);
        free(head);
        return cur;
    }
    pre = head;

    while (cur) {
        if(cur->element->id == id) {
            pre->next = cur->next;
            free(cur->element);
            free(cur);
            break;
        }
        pre = cur;
        cur = cur->next;
    }
    return head;
}

void ll_free(struct node *head) {
    struct node *cur;
    while (head) {
        free(head->element);
        cur = head;
        head = head->next;
        free(cur);
    }
}
