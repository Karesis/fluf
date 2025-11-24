# std/list/idlist.h

## `static inline void idlist_init(idlist_t *node) {`


Initialize a list head (or a standalone node).
Points prev and next to itself.


---

## `static inline void _idlist_insert(idlist_t *prev, idlist_t *next, idlist_t *node) {`


(Internal) Insert a new node between two known nodes.


---

## `static inline void idlist_add_tail(idlist_t *head, idlist_t *node) {`


Add a new node to the tail of the list.
(Insert before head)


---

## `static inline void idlist_add_head(idlist_t *head, idlist_t *node) {`


Add a new node to the head of the list.
(Insert after head)


---

## `static inline void idlist_del(idlist_t *node) {`


Remove a node from the list and re-initialize it.
Safe to call on an already removed/initialized node if logic allows,
but usually implies node is currently in a list.


---

## `static inline bool idlist_is_empty(const idlist_t *head) {`


Check if the list is empty.
Returns true if head points to itself.


---

## `#define idlist_foreach(head, iter_node) \ for ((iter_node) = (head)->next;`


Iterate over the list.


- **`head`**: Pointer to the head (sentinel).
- **`iter_node`**: Pointer to idlist_t used as iterator.


---

## `#define idlist_foreach_safe(head, iter_node, temp_node) \ for ((iter_node) = (head)->next, (temp_node) = (iter_node)->next;`


Iterate over the list safely (allows deletion).


- **`head`**: Pointer to the head (sentinel).
- **`iter_node`**: Pointer to idlist_t iterator.
- **`temp_node`**: Pointer to idlist_t for temporary storage.


---

