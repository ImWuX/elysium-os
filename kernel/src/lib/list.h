#pragma once

#define LIST_INIT (list_t) { .next = 0, .prev = 0 }
#define LIST_INIT_CIRCULAR(NAME) (list_t) { .next = &(NAME), .prev = &(NAME) }

typedef struct list_element {
    struct list_element *next;
    struct list_element *prev;
} list_element_t;

typedef struct list_element list_t;

/**
 * @brief Insert a new element behind an existing element in a list.
 * @param position element to insert behind
 * @param element
 */
void list_append(list_element_t *position, list_element_t *element);

/**
 * @brief Insert a new element before an existing element in a list.
 * @param position element to insert before
 * @param element
 */
void list_prepend(list_element_t *position, list_element_t *element);

/**
 * @brief Delete a element from a list.
 * @param element
 */
void list_delete(list_element_t *element);

/**
 * @brief Test if a list is empty.
 * @param list list head
 * @returns true = list is empty
 */
bool list_is_empty(list_t *list);

/**
 * @brief Get the structure/container that the element is embedded in.
 * @param ELEMENT embedded element
 * @param TYPE type of container
 * @param MEMBER name of the list embedded in container
 * @returns pointer to container
 */
#define LIST_CONTAINER_GET(ELEMENT, TYPE, MEMBER) ((TYPE *) ((uintptr_t) (ELEMENT) - __builtin_offsetof(TYPE, MEMBER)))

/**
 * @brief Iterate over a list.
 * @param LIST list_t to iterate over
 * @param ELEM_PTR list_element_t* to be used as iterator
 */
#define LIST_FOREACH(LIST, ELEM_PTR) for((ELEM_PTR) = (LIST)->next; (ELEM_PTR) && (ELEM_PTR) != (LIST); (ELEM_PTR) = (ELEM_PTR)->next)

/**
 * @brief Get the next element in a list.
 * @param ELEMENT
 */
#define LIST_NEXT(ELEMENT) ((ELEMENT)->next)

/**
 * @brief Get the previous element in a list.
 * @param ELEMENT
 */
#define LIST_PREVIOUS(ELEMENT) ((ELEMENT)->prev)