#include "list.h"

void list_insert_behind(list_t *position, list_t *node) {
    node->prev = position;
    node->next = position->next;
    if(position->next) position->next->prev = node;
    position->next = node;
}

void list_insert_before(list_t *position, list_t *node) {
    node->next = position;
    node->prev = position->prev;
    if(position->prev) position->prev->next = node;
    position->prev = node;
}

void list_delete(list_t *node) {
    if(node->prev) node->prev->next = node->next;
    if(node->next) node->next->prev = node->prev;
}

bool list_is_empty(list_t *list) {
    return !list->next || list == list->next;
}