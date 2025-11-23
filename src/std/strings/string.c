#include <std/strings/string.h>
#include <core/math.h>
#include <core/msg.h>
#include <stdio.h> /// vsnprintf
#include <stdarg.h> /// va_list

/* --- Internals --- */

/// we always need space for null terminator.
/// real capacity needed = requested + 1.
static bool string_grow(string_t *s, usize needed)
{
	/// 1. calculate new capacity (double strategy)
	usize new_cap = (s->cap == 0) ? 16 : s->cap * 2;

	/// if explicit need is larger, satisfy it
	/// needed includes the new content but NOT the null terminator yet
	if (new_cap < needed + 1) {
		new_cap = next_power_of_two(needed + 1);
	}

	/// 2. realloc
	layout_t old_l = layout(s->cap, 1);
	layout_t new_l = layout(new_cap, 1); /// strings align to 1

	char *new_data = (char *)allocer_realloc(s->alc, s->data, old_l, new_l);
	if (!new_data)
		return false;

	s->data = new_data;
	s->cap = new_cap;

	/// ensure null termination if it was fresh
	/// (realloc preserves old data, so if we extend, s->data[s->len] is still valid)
	return true;
}

/* --- Lifecycle --- */

bool string_init(string_t *s, allocer_t alc, usize cap_hint)
{
	s->alc = alc;
	s->len = 0;
	s->cap = 0;
	s->data = nullptr;

	if (cap_hint > 0) {
		if (!string_grow(s, cap_hint))
			return false;
		s->data[0] = '\0';
	}
	return true;
}

void string_deinit(string_t *s)
{
	if (s->data) {
		layout_t l = layout(s->cap, 1);
		allocer_free(s->alc, s->data, l);
	}
	s->data = nullptr;
	s->len = 0;
	s->cap = 0;
}

string_t *string_new(allocer_t alc, usize cap_hint)
{
	/// 1. alloc header
	string_t *s = (string_t *)allocer_alloc(alc, layout_of(string_t));
	if (!s)
		return nullptr;

	/// 2. init body
	if (!string_init(s, alc, cap_hint)) {
		allocer_free(alc, s, layout_of(string_t));
		return nullptr;
	}
	return s;
}

void string_drop(string_t *s)
{
	if (s) {
		allocer_t alc = s->alc;
		string_deinit(s);
		allocer_free(alc, s, layout_of(string_t));
	}
}

/* --- Modification --- */

bool string_push(string_t *s, char c)
{
	/// len + 1 (for c) + 1 (for \0) <= cap
	if (s->len + 2 > s->cap) {
		if (!string_grow(s, s->len + 1))
			return false;
	}

	s->data[s->len] = c;
	s->len++;
	s->data[s->len] = '\0'; /// invariant
	return true;
}

bool string_append(string_t *s, str_t slice)
{
	if (slice.len == 0)
		return true;

	usize new_len;
	if (checked_add(s->len, slice.len, &new_len))
		return false;

	if (new_len + 1 > s->cap) {
		if (!string_grow(s, new_len))
			return false;
	}

	memcpy(s->data + s->len, slice.ptr, slice.len);
	s->len = new_len;
	s->data[s->len] = '\0';
	return true;
}

bool string_append_cstr(string_t *s, const char *cstr)
{
	return string_append(s, str_from_cstr(cstr));
}

bool string_fmt(string_t *s, const char *fmt, ...)
{
	va_list args;

	/// 1. try with a small buffer on stack or dry run?
	/// standard trick: snprintf(nullptr, 0, ...) returns needed size.

	va_start(args, fmt);
	int needed = vsnprintf(nullptr, 0, fmt, args);
	va_end(args);

	if (needed < 0)
		return false; /// format error

	/// 2. reserve space
	usize u_needed = (usize)needed;
	usize new_len;
	if (checked_add(s->len, u_needed, &new_len))
		return false;

	if (new_len + 1 > s->cap) {
		if (!string_grow(s, new_len))
			return false;
	}

	/// 3. write
	va_start(args, fmt);
	/// we write to s->data + s->len
	vsnprintf(s->data + s->len, u_needed + 1, fmt, args);
	va_end(args);

	s->len = new_len;
	/// vsnprintf adds \0 automatically
	return true;
}

void string_clear(string_t *s)
{
	s->len = 0;
	if (s->data)
		s->data[0] = '\0';
}

bool string_reserve(string_t *s, usize additional)
{
	usize needed;
	/// check overflow: len + additional
	if (checked_add(s->len, additional, &needed))
		return false;

	/// we need space for terminator too
	if (needed + 1 > s->cap) {
		return string_grow(s, needed);
	}
	return true;
}
