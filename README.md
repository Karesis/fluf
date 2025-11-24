# fluf

![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)
![Standard](https://img.shields.io/badge/std-C23-purple.svg)
![C/C++ CI](https://github.com/Karesis/fluf/actions/workflows/ci.yml/badge.svg)

**A practical, debuggable, and opinionated C23 toolkit for compilers and systems programming.**

`fluf` is a foundational library designed to break the cycle of complex abstractions. It rejects "template-in-C" macro magic in favor of concrete, simple, and highly debuggable tools tailored for high-performance tasks like building compilers, tools, and emulators.

> **Status:** **v0.3.0 (Feature Complete).** The Core infrastructure and Standard Library (including IO, System, and Unicode support) are fully implemented and tested.

## Core Philosophy

The design of `fluf` is a direct response to the "fake prosperity" of overly complex C libraries.

* **Debuggability First:** No magic. Core structures are hand-written C. You can step-in with GDB and see exactly what's happening.
* **Concrete over Generic:** We prefer `void*` containers (like `Vec<void*>`) over complex macro-templated code. In compiler dev, you are mostly storing pointers (`AstNode*`, `Symbol*`) anyway.
* **Allocator-Driven:** All memory management goes through an abstract `allocer_t` v-table interface, allowing seamless switching between Heap and Bump/Arena allocators.
* **Stack-Based Lifecycle:** Stateful objects prefer an `_init() / _destroy()` lifecycle on the stack to avoid unnecessary heap allocations and lifetime bugs.

## Features

### Core Infrastructure (`include/core/`)
* **Type System:** Primitive aliases, safe casting macros, and `fmt()` generics.
* **Memory:** `allocer_t` v-table interface and `layout_t`.
* **Error Handling:** `Result<T,E>` and `Option<T>` monads.
* **Testing:** Header-only test framework with Process Isolation (Death Tests).
* **Hashing:** FNV-1a 64-bit implementation.

### Standard Library (`include/std/`)

#### Memory & Containers
* **Allocators:**
    * `allocer_system()`: Cross-platform (POSIX/Windows) system heap wrapper.
    * `bump_t`: High-performance arena allocator with "Keep-the-Tip" reset strategy.
* **Containers:**
    * `vec(T)`: Type-safe dynamic array (macro-wrapped, void* backed).
    * `map(K, V)`: Open-addressing hash map with linear probing and tombstone support.
    * `idlist_t`: Intrusive circular doubly linked list.
    * `bitset_t`: Dense bitset optimized with word-level operations and intrinsics.

#### String & Text
* **Strings:**
    * `str_t`: Non-owning string slice (View) with zero-copy splitting/trimming.
    * `string_t`: Owned, growable string builder ensuring null-termination.
    * `interner_t`: String Interner (Symbol Table) using Bump allocation for stable storage.
* **Unicode:**
    * `utf8`: Secure decoder/encoder handling overlong sequences and surrogates.
    * `prop`: Binary-search based character properties (XID, WhiteSpace).

#### System & I/O
* **FileSystem (`fs`):**
    * `file`: Zero-copy read-to-string and atomic write helpers.
    * `path`: Cross-platform path builder and query utilities.
    * `dir`: Recursive directory walker (POSIX `opendir` / Windows `FindFirstFile`).
    * `srcmanager`: Source file manager mapping global offsets to file/line/col (Diagnostic infrastructure).
* **Environment (`env`):**
    * `args`: Iterator-based command line argument parser.
    * `env`: Cross-platform environment variable getter/setter.

## Getting Started

### Prerequisites

* Clang (Recommended) or GCC supporting **C23**.
* Make.

### Building

`fluf` is built as a static library.

```bash
# Build lib/libfluf.a
make

# Run the test suite (verifies all core modules)
make test

# Clean build artifacts
make clean
````

### Example: Symbol Table Simulation

This example demonstrates how `fluf` components work together to build a high-performance symbol table, typical in compiler development.

```c
#include <std/vec.h>
#include <std/map.h>
#include <std/strings/intern.h>
#include <std/allocers/system.h>
#include <stdio.h>

// 1. Define generic containers
// Map: Symbol ID -> Integer Value (e.g., variable value)
defMap(symbol_t, int, SymbolMap);
// Vec: List of active symbols
defVec(symbol_t, SymbolList);

int main(void) {
    // Setup memory
    allocer_t sys = allocer_system();
    
    // Initialize Interner (Symbol Table) and Containers
    interner_t interner;
    intern_init(&interner, sys); // Uses internal Bump allocator for strings

    SymbolMap values;
    map_init(values, sys, MAP_OPS_USIZE); // symbol_t is just a wrapper around u32/size

    SymbolList active_vars;
    vec_init(active_vars, sys, 0);

    // --- Simulation ---

    // 1. Intern strings (Lexing phase)
    // "foo" and "bar" are stored in the interner's arena.
    // `s1` and `s2` are just u32 IDs.
    symbol_t s1 = intern_cstr(&interner, "foo");
    symbol_t s2 = intern_cstr(&interner, "bar");
    symbol_t s3 = intern_cstr(&interner, "foo"); // Reuse!

    // Deduplication check
    if (sym_eq(s1, s3)) {
        printf("Interner working: 'foo' has same ID (%u)\n", s1.id);
    }

    // 2. Assign values (Parsing/Analysis phase)
    map_put(values, s1, 42);
    map_put(values, s2, 100);

    // 3. Track order
    vec_push(active_vars, s1);
    vec_push(active_vars, s2);

    // 4. Iterate and Resolve
    printf("\n--- Symbol Dump ---\n");
    vec_foreach(sym_ptr, active_vars) {
        symbol_t sym = *sym_ptr;
        
        // Resolve ID -> String (O(1))
        str_t name = intern_resolve(&interner, sym);
        
        // Retrieve Value (O(1))
        int *val = map_get(values, sym);

        printf("Var '" fmt_str(name) "' = %d\n", name, val ? *val : 0);
    }

    // Cleanup (RAII-style usually, but explicit here)
    vec_deinit(active_vars);
    map_deinit(values);
    intern_deinit(&interner); // Frees all string memory at once!
    
    return 0;
}
```

## Documentation

API documentation is generated using [**cnote**](https://github.com/Karesis/cnote).
See [SUMMARY.md](./docs/reference/SUMMARY.md) for generated markdown files.

## License

This project is licensed under the **Apache-2.0 License**. See the [LICENSE](./LICENSE) file for details.

