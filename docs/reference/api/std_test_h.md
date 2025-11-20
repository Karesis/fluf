# std/test.h

## `#define expect(cond) \ do {`


Basic test assertion.
If fails, prints error and returns from the current test function.


---

## `#define expect_eq(expected, actual) \ do {`


Generic Equality Assertion.


---

## `static inline bool _test_check_panic_status(int status, const char *file, int line) {`


Internal helper to analyze the child process exit status.


- **`status`**: The status code returned by waitpid.
- **`file`**: The source file name (for error reporting).
- **`line`**: The line number (for error reporting).
- **Returns**: true if the process aborted (SIGABRT), false otherwise.


---

## `#define expect_panic(stmt) \ do {`


Death Test Assertion.
Expects the expression `stmt` to cause the program to panic (abort).

Logic:
1. Fork a child process.
2. Child executes `stmt`.
3. Parent waits and uses `_test_check_panic_status` to verify the outcome.


---

## `#define RUN(name) \ do {`


Run a test case.

**Example:**

```c
 * int main() 
 * {
 *     RUN(my_feature);
 * }
 
```

---

## `#define SUMMARY() \ do {`


Print summary at the end of main.


---

## `#define silence(stmt) \ do {`


Run code block with stderr silenced.
* Logic:
1. Save the current stderr file descriptor (dup).
2. Redirect stderr to /dev/null (freopen).
3. Run the statement.
4. Restore stderr from the saved descriptor (dup2).


---

