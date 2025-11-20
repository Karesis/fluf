# fluf

![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)
![Standard](https://img.shields.io/badge/std-C23-purple.svg)
![C/C++ CI](https://github.com/Karesis/fluf/actions/workflows/ci.yml/badge.svg)

**A practical, debuggable, and opinionated C23 toolkit for compilers and systems programming.**

`fluf` is a foundational library designed to break the cycle of complex abstractions. It rejects "template-in-C" macro magic in favor of concrete, simple, and highly debuggable tools tailored for high-performance tasks like building compilers, tools, and emulators.

> **Note:** This project is currently undergoing a major refactor. The **Core** infrastructure (Types, Memory Layout, Allocator Interface, Testing) is stable. The **Standard** library (Vectors, Strings, Bump Allocators) is actively being ported.

## Core Philosophy

The design of `fluf` is a direct response to the "fake prosperity" of overly complex C libraries.

* **Debuggability First:** No magic. Core structures are hand-written C. You can step-in with GDB and see exactly what's happening.
* **Concrete over Generic:** We prefer `void*` containers (like `Vec<void*>`) over complex macro-templated code. In compiler dev, you are mostly storing pointers (`AstNode*`, `Symbol*`) anyway.
* **Allocator-Driven:** All memory management goes through an abstract `allocer_t` v-table interface, allowing seamless switching between Heap and Bump/Arena allocators.
* **Stack-Based Lifecycle:** Stateful objects prefer an `_init() / _destroy()` lifecycle on the stack to avoid unnecessary heap allocations and lifetime bugs.

## Features (Current Refactor Status)

### Core Infrastructure (`include/core/`)

The bedrock of the library is fully implemented and tested:

* **Type System (`type.h`):** Rust-style primitive aliases (`u8`, `usize`, `i32`), safe casting, and `fmt()` generics.
* **Memory Management (`mem/`):**
  * `layout_t`: A standardized struct for memory requirements (size + alignment).
  * `allocer_t`: A v-table interface (Fat Pointer) for polymorphic memory allocators.
* **Error Handling (`result.h`):** Rust-inspired `Result<T, E>` and `Option<T>` with `try()` and `if_let` macros for control flow.
* **Metaprogramming (`macros.h`):** Type introspection (`is_pointer`, `is_array`), `container_of`, and safety checks.
* **Testing (`std/test.h`):** A header-only, GoogleTest-inspired testing framework supporting **Death Tests** (`expect_panic`).

### Roadmap (Porting from Main)

* **Allocators:** Bump/Arena allocators.
* **Containers:** `vec_t` (Dynamic Array), `strhashmap_t` (Linear Probing Map).
* **Strings:** `strslice_t` (String View), `string_t` (Builder), `strintern_t` (Interner).
* **IO:** File reading/writing utilities.

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
```

### Example: Result Handling & Allocator Interface

Since the high-level containers are WIP, here is how the Core modules work today:

```c
#include <core/result.h>
#include <core/mem/allocer.h>
#include <stdio.h>

// Define a Result type: returns i32 on success, const char* on error
defResult(i32, const char*, ResInt);

ResInt safe_divide(i32 a, i32 b) {
    if (b == 0) return (ResInt)err("Division by zero");
    return (ResInt)ok(a / b);
}

int main(void) {
    ResInt r = safe_divide(10, 2);

    // Rust-style if_let syntax
    if_let(val, r) {
        printf("Result: %d\n", val);
    } else {
        printf("Error: %s\n", r.err);
    }

    return 0;
}
```

## Documentation

API documentation is generated using [**cnote**](https://github.com/Karesis/cnote).
See [SUMMARY.md](./docs/reference/SUMMARY.md) for generated markdown files.

## License

This project is licensed under the **Apache-2.0 License**. See the [LICENSE](./LICENSE) file for details.
