#include <std/fs/dir.h>
#include <std/strings/string.h>
#include <std/fs/path.h>
#include <string.h>

/*
 * ==========================================================================
 * Internal Recursive Logic
 * ==========================================================================
 */

/// forward declaration of platform specific body
static bool _dir_walk_recursive(string_t *path_builder, dir_walk_cb cb,
				void *userdata);

bool dir_walk(allocer_t alc, const char *root, dir_walk_cb cb, void *userdata)
{
	string_t path;
	if (!string_init(&path, alc, 256))
		return false; /// reasonable start capacity

	/// seed the builder with root
	if (!string_append_cstr(&path, root)) {
		string_deinit(&path);
		return false;
	}

	/// start recursion
	bool res = _dir_walk_recursive(&path, cb, userdata);

	string_deinit(&path);
	return res;
}

/*
 * ==========================================================================
 * Platform Specific Implementation
 * ==========================================================================
 */

#if defined(_WIN32)

#include <windows.h>

static bool _dir_walk_recursive(string_t *path, dir_walk_cb cb, void *userdata)
{
	/// windows requires appending "\*" to list files: "folder\*"
	usize original_len = string_len(path);

	if (!path_push(path, str("*")))
		return false; /// helper from path.h

	WIN32_FIND_DATA find_data;
	HANDLE hFind = FindFirstFile(string_cstr(path), &find_data);

	/// restore path to "folder" (remove the *)
	/// we cheat by setting len because we know path_push only added chars
	/// but to be safe/clean, we should construct the search string separately?
	/// for efficiency, let's just truncate back.
	/// string_truncate(path, original_len); /// assuming we implement this or set len
	path->len = original_len;
	path->data[original_len] = '\0';

	if (hFind == INVALID_HANDLE_VALUE) {
		return false; /// directory not found or error
	}

	bool cont = true;
	do {
		const char *name = find_data.cFileName;

		/// skip . and ..
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}

		/// append name: "folder/file"
		if (!path_push(path, str_from_cstr(name))) {
			cont = false;
			break;
		}

		dir_entry_type_t type = (find_data.dwFileAttributes &
					 FILE_ATTRIBUTE_DIRECTORY) ?
						DIR_ENTRY_DIR :
						DIR_ENTRY_FILE;

		/// callback
		if (!cb(string_cstr(path), type, userdata)) {
			cont = false; /// abort requested
		} else if (type == DIR_ENTRY_DIR) {
			/// recurse
			if (!_dir_walk_recursive(path, cb, userdata)) {
				cont = false;
			}
		}

		/// backtrack: restore path length for next iteration
		path->len = original_len;
		path->data[original_len] = '\0';

	} while (cont && FindNextFile(hFind, &find_data) != 0);

	FindClose(hFind);
	return cont;
}

#else

/// --- POSIX (Linux/macOS) ---
#include <dirent.h>
#include <sys/stat.h>

static bool _dir_walk_recursive(string_t *path, dir_walk_cb cb, void *userdata)
{
	DIR *d = opendir(string_cstr(path));
	if (!d)
		return false;

	struct dirent *entry;
	usize original_len = string_len(path);
	bool cont = true;

	while (cont && (entry = readdir(d)) != NULL) {
		const char *name = entry->d_name;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}

		/// build full path: "root/entry"
		/// path_push automatically handles separator
		if (!path_push(path, str_from_cstr(name))) {
			cont = false;
			break;
		}

		/// determine type
		dir_entry_type_t type = DIR_ENTRY_UNKNOWN;
#ifdef _DIRENT_HAVE_D_TYPE
		if (entry->d_type == DT_DIR)
			type = DIR_ENTRY_DIR;
		else if (entry->d_type == DT_REG)
			type = DIR_ENTRY_FILE;
#endif
		/// fallback if d_type is unknown (or system doesn't support it)
		if (type == DIR_ENTRY_UNKNOWN) {
			struct stat st;
			if (stat(string_cstr(path), &st) == 0) {
				if (S_ISDIR(st.st_mode))
					type = DIR_ENTRY_DIR;
				else if (S_ISREG(st.st_mode))
					type = DIR_ENTRY_FILE;
			}
		}

		/// callback
		if (!cb(string_cstr(path), type, userdata)) {
			cont = false;
		} else if (type == DIR_ENTRY_DIR) {
			/// recurse
			if (!_dir_walk_recursive(path, cb, userdata)) {
				cont = false;
			}
		}

		/// backtrack: Trim back to original root for next sibling
		/// "root/entry" -> "root"
		path->len = original_len;
		path->data[original_len] = '\0';
	}

	closedir(d);
	return cont;
}

#endif
