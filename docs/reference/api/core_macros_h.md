# core/macros.h

## `#define container_of(ptr, type, member) \ ({`


Get the pointer to the container struct from a pointer to one of its members.


- **`ptr`**: A pointer to the 'member' field within the container struct.
- **`type`**: The type of the container struct (e.g., struct MyStruct).
- **`member`**: The name (identifier) of the member field within the container struct.


**Example:**

```c
 * /// for assert
 * #include <assert.h>
 * /// for offsetof
 * #include <stddef.h>
 *
 * struct list_node 
 * {
 *		struct list_node *prev;
 *		struct list_node *next;
 * };
 *
 * struct my_data
 * {
 *		int id;
 *		struct list_node node;
 *		char* name;
 * };
 *
 * void container_of_demo()
 * {
 *		struct my_data item_a;
 *		item_a.id = 100;
 *		struct list_node *ptr_to_member = &item_a.node;
 *		struct my_data *ptr_to_container = container_of(ptr_to_member, struct my_data, node);
 *
 *		assert(ptr_to_container == &item_a);
 *		assert(ptr_to_container->id == 100);
 * }
 *
```


> **Note:** In C, when we do $ptr - N$, it will be translated to $address(ptr) - (N \times sizeof(type))$.
So here the `type` above need to be byte (a.k.a, char) to ensure we use `sizeof(char)` (1) to be the  base unit.


---

## `#define array_size(arr) \ ({`


Compute the element count of an array.


- **`arr`**: The array


---

