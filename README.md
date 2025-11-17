# fluf

A practical, debuggable, and opinionated C23 toolkit for compilers and systems programming.

[](https://www.google.com/search?q=https://github.com/karesis/fluf)
[](https://www.google.com/search?q=https://github.com/karesis/fluf/blob/main/LICENSE)

`fluf` is a foundational library (like a `core` or `std` library) designed for C23. It is born from the hard-earned lesson that **complex abstractions are a trap**.

This library intentionally avoids the "template-in-C" macro magic (like `DEFINE_VECTOR(T)`) common in other C libraries. Instead, `fluf` provides concrete, simple, and highly debuggable tools tailored for high-performance tasks like building compilers, tools, and emulators.

## Core Philosophy

The design of `fluf` is a direct response to the "fake prosperity" of overly complex C libraries, which often lead to opaque errors, difficult debugging, and code bloat.

The philosophy of fluf is:

1.  **Debuggability First:** No magic. All core data structures (`vec_t`, `string_t`, `strhashmap_t`) are real, hand-written C implementations. You can `step-in` with GDB and see exactly what's happening.
2.  **Concrete over Generic:** Instead of a complex, `memcpy`-based `DEFINE_VECTOR(T)` macro, `fluf` provides a single, highly-optimized `vec_t` (a `Vec<void*>`). In compiler development, you are almost always storing pointers (`AstNode*`, `Symbol*`, `Type*`), and a `Vec<void*>` is the simplest, fastest, and most honest C-native solution.
3.  **Allocator-Driven (V-Table)**: All dynamic memory is handled by an abstract `allocer_t` interface (a v-table). This allows all components (`vec_t`, `strhashmap_t`, etc.) to use any allocator—most commonly, a high-speed bump/arena allocator.
4.  **Stack-Based Lifecycle:** All stateful objects (`bump_t`, `vec_t`, `strhashmap_t`, `strintern_t`) prefer an `_init()` / `_destroy()` lifecycle on stack-allocated structs. This avoids unnecessary heap allocations for the objects themselves and fixes critical lifetime bugs associated with arenas.

## Features

`fluf` is built as a static library (`libfluf.a`) and provides a set of highly-integrated modules.

### Core (`include/core/`)

  * **`core/mem/allocer.h`**: The abstract `allocer_t` v-table interface.
  * **`core/span.h`**: A generic `span_t` struct (`{ start, end }`) for ranges.
  * **`core/msg/asrt.h`**: `asrt_msg()` - A simple, `printf`-style assert macro that (correctly) disappears when `NDEBUG` is defined.
  * **`core/msg/dbg.h`**: `dbg()` - A `printf`-style debug macro that prints `[FILE:LINE] func(): ...` and disappears when `NDEBUG` is defined.

### Standard Library (`include/std/`)

  * **Memory (`std/allocer/bump/bump.h`)**: A high-performance, multi-chunk **Bump (Arena) Allocator** (`bump_t`).
  * **Strings**
      * **`std/string/str_slice.h`**: A `strslice_t` (`{ ptr, len }`) view, the C-equivalent of `StringView` or `&str`.
      * **`std/string/string.h`**: A `string_t` dynamic string builder (like `Vec<char>`), guaranteeing a `\0` terminator.
      * **`std/string/strintern.h`**: A `strintern_t` **String Interner**. The core of a compiler's lexer; it de-duplicates strings, turning `strslice_t` inputs into unique `const char*` outputs.
  * **Containers**
      * **`std/vec.h`**: A `vec_t` dynamic array, optimized for storing `void*`.
      * **`std/hashmap/strhashmap.h`**: A `strhashmap_t`, an open-addressing, linear-probing hash map for `const char*` -\> `void*`.
  * **I/O (`std/io/file.h`)**
      * `read_file_to_slice()`: Reads an entire file into `allocer_t`-managed memory.
      * `write_file_bytes()`: Writes a block of memory to a file.
  * **Diagnostics (`std/io/sourcemap.h`)**
      * `sourcemap_t`: The "brain" of a compiler's error reporting. It maps a `span_t` (byte offset) back to a human-readable `filename:line:column`.
  * **Math (`std/math/bitset.h`)**
      * `bitset_t`: A high-performance bitset for data-flow analysis, supporting union, intersection, and difference.

## Quick Example

This example demonstrates the core `fluf` philosophy: stack-based `_init`, an `allocer_t` v-table, and concrete `void*` containers.

```c
// --- fluf core ---
#include <core/msg/asrt.h>
#include <core/mem/allocer.h>
#include <std/string/str_slice.h>

// --- fluf modules ---
#include <std/allocer/bump/bump.h>
#include <std/allocer/bump/glue.h>
#include <std/vec.h>
#include <std/string/strintern.h>

int main(void) {
    // 1. Create the Bump Arena (on the stack)
    bump_t arena;
    bump_init(&arena);

    // 2. Wrap it in the abstract v-table
    allocer_t alc = bump_to_allocer(&arena);

    // 3. Create the Vec and Interner (on the stack)
    //    Both will now use the *same* arena for all
    //    their internal allocations.
    vec_t vec;
    strintern_t interner;
    
    vec_init(&vec, &alc, 0);
    strintern_init(&interner, &alc, 0);

    // 4. Simulate lexing: we find a token "foo"
    strslice_t token_slice = SLICE_LITERAL("foo");

    // 5. Intern the token
    // `token1` is now a unique, \0-terminated `const char*`
    const char* token1 = strintern_intern_slice(&interner, token_slice);

    // 6. Intern it again
    const char* token2 = strintern_intern_slice(&interner, SLICE_LITERAL("foo"));

    // 7. Verify uniqueness (O(1) pointer comparison)
    asrt_msg(token1 == token2, "Interner failed!");

    // 8. Store the pointer in our Vec<void*>
    vec_push(&vec, (void*)token1);
    vec_push(&vec, (void*)token2);
    
    asrt_msg(vec_count(&vec) == 2, "Vec count failed");
    
    const char* found = (const char*)vec_get(&vec, 0);
    asrt_msg(found == token1, "Vec get failed");
    
    printf("fluf example finished successfully.\n");

    // 9. Clean up.
    // (Note: `bump_destroy` frees the string "foo"
    // and the vec's `data` array all at once.)
    strintern_destroy(&interner);
    vec_destroy(&vec);
    bump_destroy(&arena);
    
    return 0;
}
```

## Building & Testing

`fluf` is built with a simple, non-recursive `Makefile` and `clang`.

```sh
# Build the static library (lib/libfluf.a)
$ make
```

```sh
# Build and run all unit tests
$ make test
```

```sh
# Clean all build artifacts
$ make clean
```

## Documentation

The detailed API reference is generated from source comments using [`cnote`](https://github.com/Karesis/cnote)

You can browse the full API documentation by starting with the main table of contents:

* **Full API Reference (Table of Contents)**: [docs/reference/SUMMARY.md](docs/reference/SUMMARY.md)

This includes all public headers (e.g., `vec_t`, `bump_t`, `strintern_t`) with function signatures and detailed descriptions.

## License

This project is licensed under the [Apache-2.0](LICENSE).