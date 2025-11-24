#include <std/env.h>
#include <core/msg.h> /// for massert
#include <stdlib.h> /// getenv, setenv, free
#include <string.h> /// strlen
#include <errno.h> /// for errno, ERANGE

#if defined(_WIN32)
#include <direct.h>
#define getcwd _getcwd
#define setenv(k, v, o) _putenv_s(k, v)
#define unsetenv(k) _putenv_s(k, "")
#else
#include <unistd.h> /// getcwd
/// POSIX setenv/unsetenv are in stdlib.h
#endif

/*
 * ==========================================================================
 * Args Implementation
 * ==========================================================================
 */

bool args_init(args_t *out, allocer_t alc, int argc, char **argv)
{
	out->cursor = 0; /// start at 0 (program name) or 1?
	/// usually parsers want 1, but access to 0 is needed.
	/// here we allow access to everything, user calls next() to skip 0.

	if (!vec_init(out->items, alc, (usize)argc)) {
		return false;
	}
	out->alc = alc;

	for (int i = 0; i < argc; ++i) {
		/// create a slice view of the raw argv string
		/// safe because argv lives as long as main()
		str_t s = str_from_cstr(argv[i]);
		vec_push(out->items, s);
	}

	return true;
}

void args_deinit(args_t *args)
{
	vec_deinit(args->items);
	args->cursor = 0;
}

str_t args_next(args_t *args)
{
	if (args->cursor >= vec_len(args->items)) {
		return str(""); /// end of stream
	}
	return vec_at(args->items, args->cursor++);
}

str_t args_peek(args_t *args)
{
	if (args->cursor >= vec_len(args->items)) {
		return str("");
	}
	return vec_at(args->items, args->cursor);
}

bool args_has_next(const args_t *args)
{
	return args->cursor < vec_len(args->items);
}

usize args_remaining(const args_t *args)
{
	if (args->cursor >= vec_len(args->items))
		return 0;
	return vec_len(args->items) - args->cursor;
}

str_t args_program_name(const args_t *args)
{
	if (vec_len(args->items) > 0) {
		return vec_at(args->items, 0);
	}
	return str("");
}

/*
 * ==========================================================================
 * Env Vars Implementation
 * ==========================================================================
 */

bool env_get(const char *key, string_t *out)
{
	if (!key)
		return false;

	const char *val = getenv(key);
	if (!val)
		return false; /// not found

	return string_append_cstr(out, val);
}

bool env_set(const char *key, const char *value)
{
	if (!key || !value)
		return false;
#if defined(_WIN32)
	return _putenv_s(key, value) == 0;
#else
	return setenv(key, value, 1) == 0; /// 1 = overwrite
#endif
}

bool env_unset(const char *key)
{
	if (!key)
		return false;
#if defined(_WIN32)
	return _putenv_s(key, "") ==
	       0; /// windows doesn't strictly have unsetenv
#else
	return unsetenv(key) == 0;
#endif
}

/*
 * ==========================================================================
 * Process Info
 * ==========================================================================
 */

bool env_current_dir(string_t *out)
{
	/// start with a reasonable guess (e.g., 1024 bytes).
	/// most paths fit in this, avoiding realloc loops in 99% cases.
	usize guess_size = 1024;

	while (true) {
		/// 1. reserve space in the string buffer.
		/// string_reserve handles the check against cap and reallocs if needed.
		if (!string_reserve(out, guess_size)) {
			return false; /// oOM
		}

		/// 2. calculate where to write.
		/// we want to APPEND, so we write at data + len.
		char *buffer_ptr = out->data + out->len;

		/// calculate available space.
		/// string_reserve guarantees at least 'guess_size' bytes are available,
		/// plus the slot for null terminator.
		/// getcwd expects the size including the null terminator.
		usize available_cap = out->cap - out->len;

		/// 3. try getcwd directly into the string buffer.
		if (getcwd(buffer_ptr, (int)available_cap) != NULL) {
			/// success!
			/// getcwd writes a null-terminated string.
			/// we need to update the string's length manually because
			/// string_t doesn't know we touched its private parts.

			usize added_len = strlen(buffer_ptr);
			out->len += added_len;
			/// invariant check: string_t expects data[len] == '\0', which getcwd did.
			return true;
		}

		/// 4. handle Failure
		if (errno == ERANGE) {
			/// buffer was too small. Double the guess and retry.
			/// check for overflow before doubling
			if (guess_size > (usize)-1 / 2)
				return false; /// prevent infinite loop
			guess_size *= 2;
			continue;
		} else {
			/// other errors (EACCES, etc.)
			return false;
		}
	}
}
