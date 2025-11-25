/*
 *    Copyright 2025 Karesis
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <std/fs.h>
#include <core/msg.h> // for massert (if needed) or debug logs
#include <stdio.h> // for fopen, fread, etc.

/// --- Platform Headers ---
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/*
 * ==========================================================================
 * Metadata Ops
 * ==========================================================================
 */

bool file_exists(const char *path)
{
	if (!path)
		return false;
	FILE *f = fopen(path, "rb");
	if (f) {
		fclose(f);
		return true;
	}
	return false;
}

bool file_remove(const char *path)
{
	if (!path)
		return false;
	return remove(path) == 0;
}

bool fs_is_dir(const char *path)
{
	if (!path)
		return false;

#if defined(_WIN32)
	DWORD attrs = GetFileAttributesA(path);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;
	/// check if set DIRECTORY
	return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat s;
	/// stat will follow symlinks
	/// if point to a path, here return 0 and S_ISDIR is true
	if (stat(path, &s) != 0)
		return false;
	return S_ISDIR(s.st_mode);
#endif
}

bool fs_is_file(const char *path)
{
	if (!path)
		return false;

#if defined(_WIN32)
	DWORD attrs = GetFileAttributesA(path);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;

	/// on windows, usually its a file, if its not a dir or device
	/// exclude them
	if (attrs & FILE_ATTRIBUTE_DIRECTORY)
		return false;
	if (attrs & FILE_ATTRIBUTE_DEVICE)
		return false;

	return true;
#else
	struct stat s;
	if (stat(path, &s) != 0)
		return false;

	/// use S_ISREG (Is Regular File)
	/// to exclude Socket, FIFO, Device, Block Device
	return S_ISREG(s.st_mode);
#endif
}

/*
 * ==========================================================================
 * I/O Ops
 * ==========================================================================
 */

bool file_read_to_string(const char *path, string_t *out)
{
	if (!path || !out)
		return false;

	FILE *f = fopen(path, "rb");
	if (!f)
		return false;

	// 1. Get file size for pre-allocation
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return false;
	}
	long fsize = ftell(f);
	if (fsize < 0) {
		fclose(f);
		return false;
	}
	rewind(f);

	// 2. Reserve space in string
	// Note: string_reserve handles overflow checks and ensures space for \0
	usize bytes_to_read = (usize)fsize;
	if (!string_reserve(out, bytes_to_read)) {
		fclose(f);
		return false; // OOM
	}

	// 3. Direct Read (Zero Copy from IO buffer to String buffer)
	// Access raw buffer: data + current_len
	char *dest_ptr = out->data + out->len;

	usize read_count = fread(dest_ptr, 1, bytes_to_read, f);

	// 4. Update String Metadata
	// Note: read_count might be less than bytes_to_read if EOF hit early (rare in "rb")
	// or error occurred.
	if (ferror(f)) {
		fclose(f);
		return false;
	}

	out->len += read_count;
	out->data[out->len] = '\0'; // Restore invariant

	fclose(f);
	return true;
}

bool file_write(const char *path, str_t content)
{
	if (!path)
		return false;

	FILE *f = fopen(path, "wb"); // Overwrite, Binary
	if (!f)
		return false;

	bool success = true;
	if (content.len > 0) {
		usize written = fwrite(content.ptr, 1, content.len, f);
		if (written != content.len) {
			success = false;
		}
	}

	fclose(f);
	return success;
}

bool file_append(const char *path, str_t content)
{
	if (!path)
		return false;

	FILE *f = fopen(path, "ab"); // Append, Binary
	if (!f)
		return false;

	bool success = true;
	if (content.len > 0) {
		usize written = fwrite(content.ptr, 1, content.len, f);
		if (written != content.len) {
			success = false;
		}
	}

	fclose(f);
	return success;
}
