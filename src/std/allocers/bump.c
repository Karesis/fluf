#include <std/allocers/bump.h>
#include <core/mem/allocer.h> /// for allocer_alloc/free
#include <core/msg.h> /// for massert
#include <core/math.h> /// for align_up, checked_add, etc.
#include <string.h> /// for memcpy, memset

/*
 * ==========================================================================
 * 1. Constants & Types
 * ==========================================================================
 */

#define CHUNK_ALIGN 16

/// calculate footer size aligned to 16 bytes
#define FOOTER_SIZE (align_up(sizeof(chunk_footer_t), CHUNK_ALIGN))

/// default usable size implies specific overhead calculations
#define DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER (4096 - FOOTER_SIZE)

/*
 * ==========================================================================
 * 2. Sentinel Empty Chunk (Optimization)
 * ==========================================================================
 * Avoids NULL checks in the hot path by pointing to a static "full" chunk.
 */

static chunk_footer_t EMPTY_CHUNK_SINGLETON;
static bool EMPTY_CHUNK_INITIALIZED = false;

static chunk_footer_t *get_empty_chunk(void)
{
	if (unlikely(!EMPTY_CHUNK_INITIALIZED)) {
		/// point data/ptr to itself so any access stays within valid (but 0-sized) memory
		EMPTY_CHUNK_SINGLETON.data_start = (u8 *)&EMPTY_CHUNK_SINGLETON;
		EMPTY_CHUNK_SINGLETON.chunk_size = 0;
		EMPTY_CHUNK_SINGLETON.prev = &EMPTY_CHUNK_SINGLETON;
		EMPTY_CHUNK_SINGLETON.ptr = (u8 *)&EMPTY_CHUNK_SINGLETON;
		EMPTY_CHUNK_SINGLETON.allocated_bytes = 0;
		EMPTY_CHUNK_INITIALIZED = true;
	}
	return &EMPTY_CHUNK_SINGLETON;
}

static bool chunk_is_empty(chunk_footer_t *footer)
{
	return footer == get_empty_chunk();
}

/*
 * ==========================================================================
 * 3. Internal Chunk Management
 * ==========================================================================
 */

static void dealloc_chunk_list(bump_t *bump, chunk_footer_t *footer)
{
	while (!chunk_is_empty(footer)) {
		chunk_footer_t *prev = footer->prev;

		/// reconstruct layout to free correctly using backing allocator
		/// we allocated `chunk_size` bytes.
		/// the alignment used was at least CHUNK_ALIGN.
		layout_t l = layout(footer->chunk_size, CHUNK_ALIGN);

		/// free the raw memory block (starts at data_start)
		allocer_free(bump->backing, footer->data_start, l);

		footer = prev;
	}
}

/**
 * @brief Allocate a new chunk.
 * * Replaces the original `aligned_malloc_internal` logic with `allocer_alloc`.
 */
static chunk_footer_t *new_chunk(bump_t *bump, usize new_size_no_footer,
				 usize align, chunk_footer_t *prev)
{
	/// round up requested size to maintain footer alignment
	new_size_no_footer = align_up(new_size_no_footer, CHUNK_ALIGN);

	usize alloc_size;
	/// safety: Check for overflow when adding footer
	if (checked_add(new_size_no_footer, FOOTER_SIZE, &alloc_size)) {
		return nullptr;
	}

	/// round up total size to requested alignment
	alloc_size = align_up(alloc_size, align);
	if (alloc_size == 0)
		return nullptr;

	/// [Dependency Injection] Use the backing allocator
	layout_t l = layout(alloc_size, align);
	u8 *data = (u8 *)allocer_alloc(bump->backing, l);

	if (!data)
		return nullptr;

	/// initialize Footer
	/// the footer is placed at the very end of the allocated block
	chunk_footer_t *footer_ptr =
		(chunk_footer_t *)(data + new_size_no_footer);

	footer_ptr->data_start = data;
	footer_ptr->chunk_size = alloc_size;
	footer_ptr->prev = prev;
	footer_ptr->allocated_bytes =
		prev->allocated_bytes + new_size_no_footer;

	/// initialize Bump Pointer
	/// starts at the footer address (high address) and grows down
	uptr ptr_start = (uptr)footer_ptr;

	/// align the starting pointer down to satisfy min_align
	footer_ptr->ptr = (u8 *)align_down(ptr_start, bump->min_align);

	/// sanity check: pointer shouldn't underflow data start
	massert(footer_ptr->ptr >= footer_ptr->data_start,
		"Bump ptr underflow setup");

	return footer_ptr;
}

/*
 * ==========================================================================
 * 4. Allocation Logic (Slow Path)
 * ==========================================================================
 * Handles allocating a new chunk when the current one is full.
 */

static anyptr alloc_layout_slow(bump_t *bump, layout_t layout)
{
	chunk_footer_t *current_footer = bump->current_chunk;

	/// 1. calculate Growth Strategy (Double size)
	usize prev_usable_size = 0;
	if (!chunk_is_empty(current_footer)) {
		prev_usable_size = current_footer->chunk_size - FOOTER_SIZE;
	}

	usize new_size_no_footer;
	/// safety: Check overflow when doubling
	if (checked_mul(prev_usable_size, 2, &new_size_no_footer)) {
		new_size_no_footer = SIZE_MAX;
	}

	/// clamp to min default
	if (new_size_no_footer < DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER) {
		new_size_no_footer = DEFAULT_CHUNK_SIZE_WITHOUT_FOOTER;
	}

	/// 2. ensure it fits the requested layout
	usize requested_align = max(layout.align, bump->min_align);
	usize requested_size = align_up(layout.size, requested_align);

	if (new_size_no_footer < requested_size) {
		new_size_no_footer = requested_size;
	}

	/// 3. check Hard Limit (Allocation Limit)
	if (bump->limit != SIZE_MAX) {
		usize allocated = current_footer->allocated_bytes;
		usize limit = bump->limit;
		usize remaining = (limit > allocated) ? (limit - allocated) : 0;

		if (new_size_no_footer > remaining) {
			if (requested_size > remaining) {
				return nullptr; /// hard OOM
			}
			new_size_no_footer =
				requested_size; /// cap to remaining
		}
	}

	/// 4. determine Chunk Alignment
	/// must respect CHUNK_ALIGN, min_align, and requested align
	usize chunk_align = max(CHUNK_ALIGN, bump->min_align);
	if (layout.align > chunk_align) {
		chunk_align = layout.align;
	}

	/// 5. allocate New Chunk
	chunk_footer_t *new_footer = new_chunk(bump, new_size_no_footer,
					       chunk_align, current_footer);

	if (!new_footer)
		return nullptr;

	bump->current_chunk = new_footer;

	/// 6. perform Allocation in New Chunk
	chunk_footer_t *footer = new_footer;
	u8 *ptr = footer->ptr;
	u8 *start = footer->data_start;

	u8 *result_ptr;
	usize aligned_size;

	massert(((uptr)ptr % bump->min_align) == 0, "Invariant violation");

	/// optimization: Check if we need extra alignment logic logic
	/// if requested alignment is smaller than min_align, standard bumping works
	if (layout.align <= bump->min_align) {
		/// align size up to min_align to keep ptr aligned
		if (checked_add(layout.size, bump->min_align - 1,
				&aligned_size))
			return nullptr;
		aligned_size = align_down(
			aligned_size,
			bump->min_align); /// wait, this logic from original is essentially align_up

		/// align up to min_align
		/// align_up(s, a) = (s + a - 1) & ~(a - 1)
		aligned_size = align_up(layout.size, bump->min_align);

		usize capacity = (usize)(ptr - start);
		massert(aligned_size <= capacity,
			"New chunk too small logic error");

		result_ptr = ptr - aligned_size;
	} else {
		/// high alignment requirement
		/// 1. calculate size aligned to target align
		aligned_size = align_up(layout.size, layout.align);

		/// 2. align the pointer DOWN first
		u8 *aligned_ptr_end = (u8 *)align_down((uptr)ptr, layout.align);

		massert(aligned_ptr_end >= start, "New chunk alignment failed");
		usize capacity = (usize)(aligned_ptr_end - start);
		massert(aligned_size <= capacity,
			"New chunk too small logic error 2");

		result_ptr = aligned_ptr_end - aligned_size;
	}

	massert(((uptr)result_ptr % layout.align) == 0, "Result not aligned");
	massert(result_ptr >= start, "Result underflow");

	footer->ptr = result_ptr;
	return (anyptr)result_ptr;
}

/*
 * ==========================================================================
 * 5. Allocation Logic (Fast Path)
 * ==========================================================================
 */

static anyptr try_alloc_layout_fast(bump_t *bump, layout_t layout)
{
	chunk_footer_t *footer = bump->current_chunk;
	u8 *ptr = footer->ptr;
	u8 *start = footer->data_start;
	usize min_align = bump->min_align;

	u8 *result_ptr;
	usize aligned_size;

	massert(chunk_is_empty(footer) || ((uptr)ptr % min_align) == 0,
		"Bump ptr invariant broken");

	/// case A: Requested alignment <= Minimum alignment
	if (layout.align <= min_align) {
		/// we only need to ensure we decrement by a multiple of min_align
		aligned_size = align_up(layout.size, min_align);

		usize capacity = (usize)(ptr - start);
		if (aligned_size > capacity)
			return nullptr;

		result_ptr = ptr - aligned_size;
	}
	/// case B: Requested alignment > Minimum alignment
	else {
		/// we need to align the size AND the pointer
		aligned_size = align_up(layout.size, layout.align);

		/// align ptr DOWN to requested alignment
		u8 *aligned_ptr_end = (u8 *)align_down((uptr)ptr, layout.align);

		if (aligned_ptr_end < start)
			return nullptr;

		usize capacity = (usize)(aligned_ptr_end - start);
		if (aligned_size > capacity)
			return nullptr;

		result_ptr = aligned_ptr_end - aligned_size;
	}

	footer->ptr = result_ptr;
	return (anyptr)result_ptr;
}

/*
 * ==========================================================================
 * 6. Public API Implementation
 * ==========================================================================
 */

void bump_init(bump_t *self, allocer_t backing, usize min_align)
{
	massert(self != nullptr, "bump_t cannot be NULL");
	massert(is_power_of_two(min_align), "min_align must be power of two");
	massert(min_align <= CHUNK_ALIGN,
		"min_align cannot exceed CHUNK_ALIGN");

	self->current_chunk = get_empty_chunk();
	self->backing = backing;
	self->limit = SIZE_MAX;
	self->allocated = 0; /// total bytes allocated via backing
	self->min_align = min_align;
}

void bump_deinit(bump_t *self)
{
	if (self) {
		dealloc_chunk_list(self, self->current_chunk);
		self->current_chunk = get_empty_chunk();
	}
}

bump_t *bump_new(allocer_t backing, usize min_align)
{
	/// alloc the struct itself from backing
	layout_t l = layout_of(bump_t);
	bump_t *bump = (bump_t *)allocer_alloc(backing, l);

	if (!bump)
		return nullptr;

	bump_init(bump, backing, min_align);
	return bump;
}

void bump_drop(bump_t *self)
{
	if (self) {
		allocer_t backing = self->backing; /// save backing
		bump_deinit(self);
		allocer_free(backing, self, layout_of(bump_t));
	}
}

void bump_reset(bump_t *self)
{
	chunk_footer_t *current_footer = self->current_chunk;
	if (chunk_is_empty(current_footer))
		return;

	/// free all older chunks
	dealloc_chunk_list(self, current_footer->prev);
	current_footer->prev = get_empty_chunk();

	/// reset pointer of current chunk
	uptr ptr_start = (uptr)current_footer;
	current_footer->ptr = (u8 *)align_down(ptr_start, self->min_align);

	/// recalculate stats (approximate)
	usize usable_size =
		(usize)((u8 *)current_footer - current_footer->data_start);
	current_footer->allocated_bytes = usable_size;
}

/* --- Alloc Core --- */

anyptr bump_alloc_layout(bump_t *self, layout_t layout)
{
	/// handle empty alloc
	if (layout.size == 0) {
		uptr ptr = (uptr)self->current_chunk->ptr;
		return (anyptr)align_down(ptr, layout.align);
	}
	/// handle alignment default
	if (layout.align == 0 || !is_power_of_two(layout.align)) {
		layout.align = 1;
	}

	/// try fast path
	anyptr alloc = try_alloc_layout_fast(self, layout);
	if (alloc)
		return alloc;

	/// fallback to slow path
	return alloc_layout_slow(self, layout);
}

anyptr bump_alloc(bump_t *self, usize size, usize align)
{
	return bump_alloc_layout(self, layout(size, align));
}

anyptr bump_alloc_copy(bump_t *self, const void *src, usize size, usize align)
{
	if (size == 0)
		return bump_alloc(self, size, align);

	anyptr dest = bump_alloc(self, size, align);
	if (dest && src) {
		memcpy(dest, src, size);
	}
	return dest;
}

char *bump_alloc_cstr(bump_t *self, const char *str)
{
	if (!str)
		return nullptr;
	usize len = strlen(str);
	return (char *)bump_alloc_copy(self, str, len + 1, 1);
}

char *bump_dup_str(bump_t *self, str_t s)
{
	if (s.len == 0) {
		/// return "", not nullptr
		char *empty = (char *)bump_alloc(self, 1, 1);
		if (empty)
			*empty = '\0';
		return empty;
	}

	/// len + 1 for '\0'
	char *ptr = (char *)bump_alloc(self, s.len + 1, 1);
	if (ptr) {
		memcpy(ptr, s.ptr, s.len);
		ptr[s.len] = '\0';
	}
	return ptr;
}

anyptr bump_zalloc(bump_t *self, layout_t layout)
{
	anyptr ptr = bump_alloc_layout(self, layout);
	if (ptr && layout.size > 0) {
		memset(ptr, 0, layout.size);
	}
	return ptr;
}

anyptr bump_realloc(bump_t *self, anyptr old_ptr, usize old_size,
		    usize new_size, usize align)
{
	if (old_ptr == nullptr) {
		return bump_alloc(self, new_size, align);
	}
	if (new_size == 0)
		return nullptr;

	anyptr new_ptr = bump_alloc(self, new_size, align);
	if (new_ptr) {
		/// minimal copy
		usize copy_size = min(old_size, new_size);
		memcpy(new_ptr, old_ptr, copy_size);
	}
	return new_ptr;
}

/* --- Capacity --- */

void bump_set_allocation_limit(bump_t *self, usize limit)
{
	self->limit = limit;
}

usize bump_get_allocated_bytes(bump_t *self)
{
	return self->current_chunk->allocated_bytes;
}

/*
 * ==========================================================================
 * 7. V-Table Implementation & Adapter
 * ==========================================================================
 */

static anyptr _bump_vt_alloc(anyptr self, layout_t layout)
{
	return bump_alloc_layout((bump_t *)self, layout);
}

static void _bump_vt_free(anyptr self, anyptr ptr, layout_t layout)
{
	unused(self);
	unused(ptr);
	unused(layout);
	/// bump allocator implies no-op free
}

static anyptr _bump_vt_realloc(anyptr self, anyptr ptr, layout_t old,
			       layout_t new_l)
{
	return bump_realloc((bump_t *)self, ptr, old.size, new_l.size,
			    new_l.align);
}

static anyptr _bump_vt_zalloc(anyptr self, layout_t layout)
{
	return bump_zalloc((bump_t *)self, layout);
}

static const allocer_vtable_t BUMP_VTABLE = {
	.alloc = _bump_vt_alloc,
	.free = _bump_vt_free,
	.realloc = _bump_vt_realloc,
	.zalloc = _bump_vt_zalloc,
};

allocer_t bump_allocer(bump_t *self)
{
	massert(self != nullptr, "bump_t cannot be NULL");
	return (allocer_t){ .self = self, .vtable = &BUMP_VTABLE };
}
