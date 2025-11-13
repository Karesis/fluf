#pragma once

#include <core/struct.h>
#include <stdbool.h>

/**
 * @brief 侵入式双向链表节点 (intrusive doubly linked list)
 *
 * `prev` 指向前一个节点，`next` 指向后一个节点。
 * 对于链表头（哨兵节点），`prev` 指向最后一个节点，`next`
 * 指向第一个节点。 对于空链表，`prev` 和 `next`
 * 都指向链表头自己。
 */
typedef struct idlist
{
  struct idlist *prev;
  struct idlist *next;
} idlist;

/**
 * @brief 初始化一个链表头 (或一个独立的节点)
 * @param list 要初始化的链表头
 */
static inline void
idlist_init(idlist *list)
{
  list->prev = list;
  list->next = list;
}

/**
 * @brief (内部) 在两个已知节点之间插入一个新节点
 * @param prev 前一个节点
 * @param next 后一个节点
 * @param node 要插入的新节点
 */
static inline void
__idlist_add(idlist *prev, idlist *next, idlist *node)
{
  next->prev = node;
  node->next = next;
  node->prev = prev;
  prev->next = node;
}

/**
 * @brief 在链表尾部添加一个新节点 (在 head->prev 之后)
 * @param head 链表头
 * @param node 要添加的节点
 */
static inline void
idlist_add_tail(idlist *head, idlist *node)
{
  __idlist_add(head->prev, head, node);
}

/**
 * @brief 在链表头部添加一个新节点 (在 head 之后)
 * @param head 链表头
 * @param node 要添加的节点
 */
static inline void
idlist_add_head(idlist *head, idlist *node)
{
  __idlist_add(head, head->next, node);
}

/**
 * @brief 从链表中删除一个节点 (并重置该节点)
 * @param node 要删除的节点
 */
static inline void
idlist_del(idlist *node)
{
  node->next->prev = node->prev;
  node->prev->next = node->next;
  idlist_init(node);
}

/**
 * @brief 检查链表是否为空
 * @param head 链表头
 * @return bool
 */
static inline bool
idlist_empty(const idlist *head)
{
  return head->next == head;
}

/**
 * @brief 获取链表中特定条目的宏
 * 这是 container_of 的一个特化版本，专门用于 idlist。
 */
#define idlist_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * @brief 遍历链表 (正向)
 * @param head 链表头
 * @param iter_node 用于迭代的 idlist* 临时变量 (如 struct
 * idlist *node)
 */
#define idlist_for_each(head, iter_node) \
  for ((iter_node) = (head)->next; (iter_node) != (head); (iter_node) = (iter_node)->next)

/**
 * @brief 遍历链表 (安全版，允许在遍历时删除节点)
 * @param head 链表头
 * @param iter_node 用于迭代的 idlist* 临时变量
 * @param temp_node 另一个 idlist* 临时变量，用于暂存 next
 * 节点
 */
#define idlist_for_each_safe(head, iter_node, temp_node)                                   \
  for ((iter_node) = (head)->next, (temp_node) = (iter_node)->next; (iter_node) != (head); \
       (iter_node) = (temp_node), (temp_node) = (iter_node)->next)
