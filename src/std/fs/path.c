#include <std/fs/path.h>
#include <string.h> /// memrchr (Linux extension) or manual loop

/* --- Implementation --- */

str_t path_ext(str_t path)
{
	if (str_is_empty(path))
		return str("");

	/// search from end
	const char *p = path.ptr + path.len - 1;
	while (p >= path.ptr) {
		if (*p == '.') {
			/// found dot. Check if it's part of a filename or just a dir dot
			/// if we hit a separator before a dot, then no extension.
			/// e.g. "dir.d/file"
			return str_from_parts(
				p + 1, (usize)(path.ptr + path.len - p - 1));
		}
		if (path_is_sep(*p)) {
			/// hit separator before dot -> No extension
			break;
		}
		p--;
	}
	return str("");
}

str_t path_file_name(str_t path)
{
	if (str_is_empty(path))
		return str("");

	const char *p = path.ptr + path.len - 1;
	while (p >= path.ptr) {
		if (path_is_sep(*p)) {
			/// found separator, everything after is filename
			return str_from_parts(
				p + 1, (usize)(path.ptr + path.len - p - 1));
		}
		p--;
	}
	/// no separator, whole string is filename
	return path;
}

str_t path_dir_name(str_t path)
{
	if (str_is_empty(path))
		return str("");

	const char *p = path.ptr + path.len - 1;
	while (p >= path.ptr) {
		if (path_is_sep(*p)) {
			/// found separator, everything before (excluding) is dir
			/// edge case: separator is at start? "/"
			if (p == path.ptr) {
				/// root
				return str_from_parts(path.ptr, 1);
			}
			return str_from_parts(path.ptr, (usize)(p - path.ptr));
		}
		p--;
	}
	/// no separator, current directory implicitly
	return str("");
}

bool path_push(string_t *buf, str_t component)
{
	if (str_is_empty(component))
		return true;

	if (!string_is_empty(buf)) {
		/// check if we need a separator
		char last = buf->data[buf->len - 1];
		if (!path_is_sep(last)) {
			if (!string_push(buf, PATH_SEP))
				return false;
		}
	}

	return string_append(buf, component);
}

bool path_set_ext(string_t *buf, str_t new_ext)
{
	/// 1. find current extension (as a view into the buffer)
	/// we can reuse logic: loop back from end
	if (string_is_empty(buf))
		return false;

	usize old_len = buf->len;
	usize cut_pos = old_len;

	char *p = buf->data + buf->len - 1;
	while (p >= buf->data) {
		if (*p == '.') {
			cut_pos = (usize)(p - buf->data);
			break;
		}
		if (path_is_sep(*p)) {
			/// no extension found, just append
			cut_pos = old_len;
			break;
		}
		p--;
	}

	/// 2. truncate
	buf->len = cut_pos; /// unsafe modification ok? string_t exposes fields.
	/// invariant: must ensure null term if we stop here?
	/// string_set_len or manually:
	buf->data[cut_pos] = '\0';

	/// 3. append dot
	if (!string_push(buf, '.'))
		return false;

	/// 4. append new ext
	return string_append(buf, new_ext);
}
