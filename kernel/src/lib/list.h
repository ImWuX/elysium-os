#pragma once

#define LIST_INIT (list_t) { .next = 0, .prev = 0 }
#define LIST_INIT_CIRCULAR(NAME) (list_t) { .next = &(NAME), .prev = &(NAME) }

typedef struct list {
    struct list *next;
    struct list *prev;
} list_t;

/**
 * @brief Insert a new node behind an existing node in a list.
 * @param position node to insert behind
 * @param node
 */
void list_insert_behind(list_t *position, list_t *node);

/**
 * @brief Insert a new node before an existing node in a list.
 * @param position node to insert before
 * @param node
 */
void list_insert_before(list_t *position, list_t *node);

/**
 * @brief Delete a node from a list.
 * @param node
 */
void list_delete(list_t *node);

/**
 * @brief Test if a list is empty.
 * @param list list head
 * @returns true = list is empty
 */
bool list_is_empty(list_t *list);

/**
 * @brief Get the structure/container that the node is embedded in.
 * @param node embedded node
 * @param type type of container
 * @param member name of the list embedded in container
 * @returns pointer to container
 */
#define LIST_GET(NODE, TYPE, MEMBER) ((TYPE *) ((uintptr_t) (NODE) - __builtin_offsetof(TYPE, MEMBER)))

/**
 * @brief Iterate over a list.
 * @param entry_ptr list_t* to be used as iterator
 * @param list list to iterate over
 */
#define LIST_FOREACH(ENTRY_PTR, LIST) for((ENTRY_PTR) = (LIST)->next; (ENTRY_PTR) && (ENTRY_PTR) != (LIST); (ENTRY_PTR) = (ENTRY_PTR)->next)