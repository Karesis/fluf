# std/env.h

## `typedef struct Args {`


Command Line Arguments Iterator.
Wraps argc/argv into a safe, iterable stream.


---

## `[[nodiscard]] bool args_init(args_t *out, allocer_t alc, int argc, char **argv);`


Parse argc/argv into an args object.


- **`out`**: Pointer to uninitialized args_t.
- **`alc`**: Allocator for the internal vector.
- **`argc`**: From main().
- **`argv`**: From main().


---

## `void args_deinit(args_t *args);`


Cleanup the args object.


---

## `str_t args_next(args_t *args);`


Get the next argument (consumes it).

- **Returns**: The argument slice, or empty string/null-ptr slice if exhausted.
Use `str_is_valid(s.ptr)` check if needed, or just check logic flow.
(Actually better: return pointer to str_t inside vec, or value).
Let's return value. If exhausted, returns str(""). Check `args_done()` first.


---

## `str_t args_peek(args_t *args);`


Peek at the next argument without consuming it.


---

## `bool args_has_next(const args_t *args);`


Check if there are more arguments.


---

## `usize args_remaining(const args_t *args);`


Get the remaining number of arguments.


---

## `str_t args_program_name(const args_t *args);`


Get the program name (argv[0]).


---

## `[[nodiscard]] bool env_get(const char *key, string_t *out);`


Get an environment variable.


- **`key`**: The env var name (e.g., "HOME", "PATH").
- **`out`**: Target string builder. The value will be APPENDED to it.
- **Returns**: true if found, false if not found or error.


> **Note:** Why append to string_t?
1. Env vars can be long.
2. `getenv` return value lifetime is platform-dependent/unsafe.
3. We want an OWNED copy for safety.


---

## `[[nodiscard]] bool env_set(const char *key, const char *value);`


Set an environment variable.

- **Returns**: true on success.


---

## `bool env_unset(const char *key);`


Unset (remove) an environment variable.


---

## `[[nodiscard]] bool env_current_dir(string_t *out);`


Get the Current Working Directory.

- **`out`**: Target string builder.
- **Returns**: true on success.


---

