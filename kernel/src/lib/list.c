#include "list.h"

void list_insert_behind(list_t *pos, list_t *node) {
    node->prev = pos;
    node->next = pos->next;
    if(pos->next) pos->next->prev = node;
    pos->next = node;
}

void list_insert_before(list_t *pos, list_t *node) {
    node->next = pos;
    node->prev = pos->prev;
    if(pos->prev) pos->prev->next = node;
    pos->prev = node;
}

void list_delete(list_t *node) {
    if(node->prev) node->prev->next = node->next;
    if(node->next) node->next->prev = node->prev;
}

bool list_is_empty(list_t *list) {
    return !list->next || list == list->next;
}