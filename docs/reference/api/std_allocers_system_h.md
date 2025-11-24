# std/allocers/system.h

## `allocer_t allocer_system(void);`


Get the global system allocator.
* @note This allocator calls OS/Libc primitives directly.
On Windows: _aligned_malloc / _aligned_free
On POSIX: posix_memalign / free


---

