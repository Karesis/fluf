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

#include <std/test.h>
#include <std/list/idlist.h>

/// define a struct that embeds the list node
typedef struct {
	int id;
	idlist_t node; /// the intrusive hook
} Item;

TEST(idlist_basic_ops)
{
	/// 1. init head
	idlist_t head;
	idlist_init(&head);
	expect(idlist_is_empty(&head));

	/// 2. create items (on stack for simplicity)
	Item i1 = { .id = 1 };
	idlist_init(&i1.node);
	Item i2 = { .id = 2 };
	idlist_init(&i2.node);
	Item i3 = { .id = 3 };
	idlist_init(&i3.node);

	/// 3. add logic
	/// list: [Head] -> i1
	idlist_add_tail(&head, &i1.node);
	expect(!idlist_is_empty(&head));

	/// list: [Head] -> i1 -> i2
	idlist_add_tail(&head, &i2.node);

	/// list: [Head] -> i3 -> i1 -> i2
	idlist_add_head(&head, &i3.node);

	/// 4. iteration verify
	int count = 0;
	idlist_t *cur;

	/// order should be: 3, 1, 2
	int expected[] = { 3, 1, 2 };

	idlist_foreach(&head, cur)
	{
		/// recover the Item pointer
		Item *item = idlist_entry(cur, Item, node);
		expect_eq(item->id, expected[count]);
		count++;
	}
	expect_eq(count, 3);

	return true;
}

TEST(idlist_deletion)
{
	idlist_t head;
	idlist_init(&head);

	Item i1 = { .id = 1 };
	idlist_init(&i1.node);
	Item i2 = { .id = 2 };
	idlist_init(&i2.node);

	idlist_add_tail(&head, &i1.node);
	idlist_add_tail(&head, &i2.node);

	/// delete i1
	idlist_del(&i1.node);

	/// verify only i2 remains
	int count = 0;
	idlist_t *cur;
	idlist_foreach(&head, cur)
	{
		Item *item = idlist_entry(cur, Item, node);
		expect_eq(item->id, 2);
		count++;
	}
	expect_eq(count, 1);

	/// verify i1 is clean (points to self)
	expect(i1.node.next == &i1.node);
	expect(i1.node.prev == &i1.node);

	return true;
}

TEST(idlist_safe_iteration)
{
	idlist_t head;
	idlist_init(&head);

	Item items[3];
	for (int i = 0; i < 3; i++) {
		items[i].id = i;
		idlist_init(&items[i].node);
		idlist_add_tail(&head, &items[i].node);
	}

	/// remove all items during iteration
	idlist_t *cur, *tmp;
	int count = 0;

	idlist_foreach_safe(&head, cur, tmp)
	{
		idlist_del(cur);
		count++;
	}

	expect_eq(count, 3);
	expect(idlist_is_empty(&head));

	return true;
}

int main()
{
	RUN(idlist_basic_ops);
	RUN(idlist_deletion);
	RUN(idlist_safe_iteration);
	SUMMARY();
}
