#pragma once

#define LIST_INIT (list_t) { .next = 0, .prev = 0 }
#define LIST_INIT_CIRCULAR(NAME) (list_t) { .next = &(NAME), .prev = &(NAME) }

typedef struct list {
    struct list *next;
    struct list *prev;
} list_t;

/**
 * @brief Insert a new node behind an existing node in a list
 *
 * @param pos Node to insert behind
 * @param node New node
 */
void list_insert_behind(list_t *pos, list_t *node);

/**
 * @brief Insert a new node before an existing node in a list
 *
 * @param pos Node to insert before
 * @param node New node
 */
void list_insert_before(list_t *pos, list_t *node);

/**
 * @brief Delete a node from a list
 *
 * @param node Node to delete
 */
void list_delete(list_t *node);

/**
 * @brief Test if a list is entry
 *
 * @param list Head of the list
 * @return true = list is empty
 */
bool list_is_empty(list_t *list);

/**
 * @brief Get the structure/container that the node is embedded in
 *
 * @param node Embedded node
 * @param type Type of container
 * @param member Name of list embedded in container
 * @return Pointer to container
 */
#define LIST_GET(NODE, TYPE, MEMBER) ((TYPE *) ((uintptr_t) (NODE) - __builtin_offsetof(TYPE, MEMBER)))

/**
 * @brief Iterate over a list
 *
 * @param entry_ptr list_t* to be used as iterator
 * @param list List to iterate over
 */
#define LIST_FOREACH(ENTRY_PTR, LIST) for((ENTRY_PTR) = (LIST)->next; (ENTRY_PTR) && (ENTRY_PTR) != (LIST); (ENTRY_PTR) = (ENTRY_PTR)->next)