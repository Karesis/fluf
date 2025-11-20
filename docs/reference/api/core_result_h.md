# core/result.h

## `#define option(T) \ bool ok;`


Define the body of an Option(T) struct.

Includes a dummy `err` field to satisfy duck-typing macros (like try/unwrap).
The `err` field is never read at runtime for Options.


> **Note:** Tag size `_TAG_SIZE_OPTION` (1) indicates Option.


---

## `#define result(T, E) \ bool ok;`


Define the body of a Result(T, E) struct.


> **Note:** Tag size `_TAG_SIZE_RESULT` (2) indicates Result.


---

## `#define unwrap(x) \ ({`


Safe unwrap. Panics if value is not present.

### Smart Logic:
1. If Option: Panic with "Option is None".
2. If Result: Panic with "Result is Err: <value>", automatically using
the correct format specifier (e.g., %d for int, %s for string) via fmt().


---

## `#define check(x, msg) \ ({`


Check the value to be present, with context.


- **`msg`**: Context message (e.g. "Failed to load config").

### Logic:
Prints: "[PANIC] ... <msg>: <error_value>"
It works for ANY error type supported by fmt() (int, char*, etc.),
not just strings!


---

## `#define unwrap_or(x, def) \ ({`


Return the inner value or a default value if None/Err.


- **`x`**: The Option/Result container.
- **`def`**: The default value to return if x is not ok.


---

## `#define if_let(var_name, expr) \ auto _container_##var_name = (expr);`


"If Let" syntax sugar.

Usage:
if_let(val, my_option) {
printf("Got %d", val);
}


---

## `#define try(expr) \ ({`


Propagate errors (The '?' operator in Rust).

Logic:
- If Ok/Some: Evaluate to the inner value.
- If Err/None: RETURN from the current function immediately.

Handling:
- Result: Returns `Err(original_err)`.
- Option: Returns `None` (zero-initializes the return struct).

@warning The return type of the current function must match the container type.


---

