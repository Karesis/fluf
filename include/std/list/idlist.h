/*
 *    Copyright 2025 Karesis
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <core/macros.h> /// for container_of
#include <stdbool.h>

/*
 * ==========================================================================
 * Intrusive Doubly Linked List
 * ==========================================================================
 * A circular, doubly linked list node meant to be embedded in other structs.
 *
 * - "Intrusive": The node is inside the data struct, meaning no extra malloc.
 * - "Circular": The end connects back to the head.
 *
 * Layout:
 * [prev] <-> [Node A] <-> [Node B] <-> [next]
 */

typedef struct IdList {
	struct IdList *prev;
	struct IdList *next;
} idlist_t;

/**
 * @brief Initialize a list head (or a standalone node).
 * Points prev and next to itself.
 */
static inline void idlist_init(idlist_t *node)
{
	node->prev = node;
	node->next = node;
}

/**
 * @brief (Internal) Insert a new node between two known nodes.
 */
static inline void _idlist_insert(idlist_t *prev, idlist_t *next,
				  idlist_t *node)
{
	next->prev = node;
	node->next = next;
	node->prev = prev;
	prev->next = node;
}

/**
 * @brief Add a new node to the tail of the list.
 * (Insert before head)
 */
static inline void idlist_add_tail(idlist_t *head, idlist_t *node)
{
	_idlist_insert(head->prev, head, node);
}

/**
 * @brief Add a new node to the head of the list.
 * (Insert after head)
 */
static inline void idlist_add_head(idlist_t *head, idlist_t *node)
{
	_idlist_insert(head, head->next, node);
}

/**
 * @brief Remove a node from the list and re-initialize it.
 * Safe to call on an already removed/initialized node if logic allows,
 * but usually implies node is currently in a list.
 */
static inline void idlist_del(idlist_t *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	idlist_init(node);
}

/**
 * @brief Check if the list is empty.
 * Returns true if head points to itself.
 */
static inline bool idlist_is_empty(const idlist_t *head)
{
	return head->next == head;
}

/**
 * @brief Helper to get the container struct from the list node.
 * Wrapper around container_of.
 */
#define idlist_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * @brief Iterate over the list.
 *
 * @param head      Pointer to the head (sentinel).
 * @param iter_node Pointer to idlist_t used as iterator.
 */
#define idlist_foreach(head, iter_node)                         \
	for ((iter_node) = (head)->next; (iter_node) != (head); \
	     (iter_node) = (iter_node)->next)

/**
 * @brief Iterate over the list safely (allows deletion).
 *
 * @param head      Pointer to the head (sentinel).
 * @param iter_node Pointer to idlist_t iterator.
 * @param temp_node Pointer to idlist_t for temporary storage.
 */
#define idlist_foreach_safe(head, iter_node, temp_node)                   \
	for ((iter_node) = (head)->next, (temp_node) = (iter_node)->next; \
	     (iter_node) != (head);                                       \
	     (iter_node) = (temp_node), (temp_node) = (iter_node)->next)
