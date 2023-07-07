#ifndef LIB_LIST_H
#define LIB_LIST_H

#define LIST_INIT { .next = 0, .prev = 0 }
#define LIST_INIT_CIRCULAR(NAME) { .next = &(NAME), .prev = &(NAME) }

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
#define list_get(node, type, member) ((type *) ((uintptr_t) (node) - offsetof(type, member)))

/**
 * @brief Iterate over a list
 *
 * @param entryptr list_t* to be used as iterator
 * @param list List to iterate over
 */
#define list_foreach(entryptr, list) for((entryptr) = (list)->next; (entryptr) && (entryptr) != (list); (entryptr) = (entryptr)->next)

#endif