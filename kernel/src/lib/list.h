#pragma once

#define LIST_INIT (list_t) { .next = 0, .prev = 0 }
#define LIST_INIT_CIRCULAR(NAME) (list_t) { .next = &(NAME), .prev = &(NAME) }

typedef struct list {
    struct list *next;
    struct list *prev;
} list_t;

/**
 * @brief Insert a new element behind an existing element in a list.
 * @param position element to insert behind
 * @param element
 */
void list_append(list_t *position, list_t *element);

/**
 * @brief Insert a new element before an existing element in a list.
 * @param position element to insert before
 * @param element
 */
void list_prepend(list_t *position, list_t *element);

/**
 * @brief Delete a element from a list.
 * @param element
 */
void list_delete(list_t *element);

/**
 * @brief Test if a list is empty.
 * @param list list head
 * @returns true = list is empty
 */
bool list_is_empty(list_t *list);

/**
 * @brief Get the structure/container that the element is embedded in.
 * @param element embedded element
 * @param type type of container
 * @param member name of the list embedded in container
 * @returns pointer to container
 */
#define LIST_GET(ELEMENT, TYPE, MEMBER) ((TYPE *) ((uintptr_t) (ELEMENT) - __builtin_offsetof(TYPE, MEMBER)))

/**
 * @brief Iterate over a list.
 * @param entry_ptr list_t* to be used as iterator
 * @param list list to iterate over
 */
#define LIST_FOREACH(ENTRY_PTR, LIST) for((ENTRY_PTR) = (LIST)->next; (ENTRY_PTR) && (ENTRY_PTR) != (LIST); (ENTRY_PTR) = (ENTRY_PTR)->next)