# core/msg.h

## `#define log_info(fmt, ...) \ do {`


Log an info message.


---

## `#define log_warn(fmt, ...) \ do {`


Log a warning message.


---

## `#define log_error(fmt, ...) \ do {`


Log an error message.


---

## `#define log_panic(fmt, ...) \ do {`


Log a panic message and abort.


> **Note:** This is for unconditional panic.

> **Note:** This macro NEVER returns.


---

## `#define todo(fmt, ...) \ do {`


Mark a piece of code as "Not Yet Implemented".


> **Note:** This macro will ALWAYS panic and abort, in both
Debug and Release builds, to prevent shipping
unfinished code.


---

## `#define dbg(fmt, ...) \ do {`


Print debug messages.


> **Note:** [DEBUG] [file:line] func(): <message>


---

## `#define dump(ptr) \ do {`


Recursively dump a struct's contents to stderr for debugging.

This macro acts like a "pretty-printer" for C structures. It utilizes
Clang's `__builtin_dump_struct` to inspect and print all fields of
the target struct automatically.


- **`ptr`**: A POINTER to the struct instance (e.g., `&my_var`).
Passing a struct by value will trigger a compile-time error.


> **Note:** **Compile-time Safety**: This macro uses `static_assert` to ensure:
1. The argument is strictly a pointer.
2. The pointed-to type is a struct or union.


> **Note:** **Zero Overhead**: In Release builds (`NDEBUG` defined), this
macro expands to `DO_NOTHING` and incurs no runtime cost.


**Example:**

```c
 * struct Rect r = { .x = 10, .y = 20 };
 * dump(&r);      /// Correct: output to stderr with file/line info
 * /// dump(r);   /// Compile Error: "Argument must be a POINTER"
 
```

---

## `#define asserrt(cond) \ do {`


Simple assertion.


> **Note:** [PANIC] [file:line] func(): Assertion Failed: (cond)

> **Note:** This macro aborts if condition is false.


---

## `#define massert(cond, fmt, ...) \ do {`


Assertion with a message.


> **Note:** [PANIC] [file:line] func(): Assertion Failed: (cond)

> **Note:** Message: <message>

> **Note:** This macro aborts if condition is false.


---

## `#define _unreachable_impl() \ do {`


Print message and panic.

This macro is used by unreach() and 
is not supposed to used as dirictly.


> **Note:** [PANIC] [file:line] func(): Reached UNREACHABLE code path


---

