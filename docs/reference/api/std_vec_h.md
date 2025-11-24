# std/vec.h

## `#define vec_new(allocator, T, capacity) \ ({`


Create a new vector on the heap (or arena).

This allocates the vector header struct using the allocator,
AND initializes the internal buffer using the same allocator.


- **`allocator`**: The allocator for both the struct and the buffer.
- **`T`**: The element type (e.g., int, struct Point).
- **`capacity`**: Initial capacity hint.
- **Returns**: A strongly-typed pointer to the new vector.


> **Note:** **Type System Usage Guide**:
1. **Local Variables (Recommended)**: Use `auto`.
@code
auto v = vec_new(sys, int, 10); // Type is preserved perfectly
vec_push(*v, 42);
@endcode

2. **Function Arguments / Struct Members**: Use `defVec`.
Since `vec(T)` generates a unique anonymous struct every time,
you cannot pass `vec(int)` across functions without a typedef.
@code
defVec(int, IntVec); // Define named type
IntVec *v = (IntVec*)vec_new(sys, int, 10); // Cast needed
@endcode

3. **Direct Assignment (Warning)**:
`vec(int) *v = vec_new(...)` will trigger "incompatible pointer types"
warning because the LHS and RHS define two distinct anonymous structs.
Use `auto` to avoid this.


---

## `#define vec_drop(v) \ do {`


Destroy a vector created with vec_new.

Frees the internal buffer AND the vector struct itself.

- **`v`**: Pointer to the vector (vec(T)*).


---

## `#define vec_push(v, val) \ ({`


Push an element to the end.

- **Returns**: true on success, false on OOM.


---

## `#define vec_at(v, i) \ ({`


Safe access with bounds checking.
@panic Panics if index is out of bounds.


---

## `#define vec_set_len(v, new_len) \ do {`


Unsafely set the length. 
Useful when manually writing to .data buffer.


---

## `#define vec_foreach(it, v) \ for (typeof((v).data) it = (v).data;`


Iterate over vector.

- **`it`**: Iterator name (pointer to element type: T*).


---

