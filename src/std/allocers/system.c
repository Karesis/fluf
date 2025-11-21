#include <std/allocers/system.h>
#include <core/msg.h>
#include <core/math.h>
#include <core/macros.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * ==========================================================================
 * Platform Specific Implementation
 * ==========================================================================
 */

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>

#define HAS_ALIGNED_REALLOC true
static inline anyptr _sys_alloc_impl(usize size, usize align)
{
	return _aligned_malloc(size, align);
}

static inline void _sys_free_impl(anyptr ptr)
{
	_aligned_free(ptr);
}

static inline anyptr _sys_realloc_impl(anyptr ptr, usize size, usize align)
{
	return _aligned_realloc(ptr, size, align);
}

#else
/// POSIX (Linux, macOS, etc.)
#include <stdlib.h>

/// POSIX does NOT support aligned realloc natively
#define HAS_ALIGNED_REALLOC false

static inline anyptr _sys_alloc_impl(usize size, usize align)
{
	/// posix_memalign requires alignment to be a power of two
	/// AND a multiple of sizeof(void *).
	if (align < sizeof(void *)) {
		align = sizeof(void *);
	}

	void *ptr = nullptr;
	int res = posix_memalign(&ptr, align, size);
	if (res != 0) {
		return nullptr;
	}
	return ptr;
}

static inline void _sys_free_impl(anyptr ptr)
{
	free(ptr);
}

[[maybe_unused]]
static inline anyptr _sys_realloc_impl(anyptr ptr, usize size, usize align)
{
	/// returning nullptr triggers the generic alloc+memcpy+free fallback
	/// in core/mem/allocer.h logic (set realloc to nullptr in vtable).
	unused(ptr);
	unused(size);
	unused(align);
	return nullptr;
}

#endif

/*
 * ==========================================================================
 *
 * V-Table Implementation
 * ==========================================================================
 */

static anyptr sys_vt_alloc(anyptr self, layout_t layout)
{
	unused(self);
	/// handle 0-size allocation safety (return a minimal ptr or nullptr depending on philosophy)
	/// c standard says malloc(0) is implementation defined.
	/// here we allocating at least 1 byte if size is 0 to maintain unique pointers,
	usize actual_size = layout.size;
	if (actual_size == 0) {
		actual_size = 1;
	}

	return _sys_alloc_impl(actual_size, layout.align);
}

static void sys_vt_free(anyptr self, anyptr ptr, layout_t layout)
{
	unused(self);
	unused(layout); /// system free usually handles size tracking itself
	if (ptr) {
		_sys_free_impl(ptr);
	}
}

#if HAS_ALIGNED_REALLOC
static anyptr sys_vt_realloc(anyptr self, anyptr ptr, layout_t old_l,
			     layout_t new_l)
{
	unused(self);
	unused(old_l);

	/// optimized path for Windows which supports aligned realloc
	/// for POSIX, _sys_realloc_impl returns nullptr, triggering the safe fallback.
	return _sys_realloc_impl(ptr, new_l.size, new_l.align);
}
#endif

static anyptr sys_vt_zalloc(anyptr self, layout_t layout)
{
	unused(self);
	/// standard `calloc` only guarantees default alignment
	/// since allocer_t demands strict alignment (which might be > 16),
	/// we cannot safely use calloc here.
	/// so use alloc + memset strategy.

	anyptr ptr = _sys_alloc_impl(layout.size, layout.align);
	if (ptr) {
		memset(ptr, 0, layout.size);
	}
	return ptr;
}

/*
 * ==========================================================================
 * Global Instance
 * ==========================================================================
 */

static const allocer_vtable_t SYSTEM_VTABLE = {
	.alloc = sys_vt_alloc,
	.free = sys_vt_free,
/// only provide realloc pointer if platform supports it.
/// if nullptr, the allocer_realloc wrapper will perform the safe fallback (Alloc+Copy+Free).
#if HAS_ALIGNED_REALLOC
	.realloc = sys_vt_realloc,
#else
	.realloc = nullptr,
#endif
	.zalloc = sys_vt_zalloc,
};

allocer_t allocer_system(void)
{
	/// stateless allocator
	return (allocer_t){ .self = nullptr, .vtable = &SYSTEM_VTABLE };
}
