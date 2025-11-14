# std/list/idlist.h

## `typedef struct idlist {`


侵入式双向链表节点 (intrusive doubly linked list)

`prev` 指向前一个节点，`next` 指向后一个节点。
对于链表头（哨兵节点），`prev` 指向最后一个节点，`next`
指向第一个节点。 对于空链表，`prev` 和 `next`
都指向链表头自己。


---

## `static inline void idlist_init(idlist *list) {`


初始化一个链表头 (或一个独立的节点)

- **`list`**: 要初始化的链表头


---

## `static inline void __idlist_add(idlist *prev, idlist *next, idlist *node) {`


(内部) 在两个已知节点之间插入一个新节点

- **`prev`**: 前一个节点
- **`next`**: 后一个节点
- **`node`**: 要插入的新节点


---

## `static inline void idlist_add_tail(idlist *head, idlist *node) {`


在链表尾部添加一个新节点 (在 head->prev 之后)

- **`head`**: 链表头
- **`node`**: 要添加的节点


---

## `static inline void idlist_add_head(idlist *head, idlist *node) {`


在链表头部添加一个新节点 (在 head 之后)

- **`head`**: 链表头
- **`node`**: 要添加的节点


---

## `static inline void idlist_del(idlist *node) {`


从链表中删除一个节点 (并重置该节点)

- **`node`**: 要删除的节点


---

## `static inline bool idlist_empty(const idlist *head) {`


检查链表是否为空

- **`head`**: 链表头
- **Returns**: bool


---

## `#define idlist_for_each(head, iter_node) \ for ((iter_node) = (head)->next;`


遍历链表 (正向)

- **`head`**: 链表头
- **`iter_node`**: 用于迭代的 idlist* 临时变量 (如 struct
idlist *node)


---

## `#define idlist_for_each_safe(head, iter_node, temp_node) \ for ((iter_node) = (head)->next, (temp_node) = (iter_node)->next;`


遍历链表 (安全版，允许在遍历时删除节点)

- **`head`**: 链表头
- **`iter_node`**: 用于迭代的 idlist* 临时变量
- **`temp_node`**: 另一个 idlist* 临时变量，用于暂存 next
节点


---

