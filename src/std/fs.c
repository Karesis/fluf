#include <std/fs.h>
#include <core/msg.h> // for massert (if needed) or debug logs
#include <stdio.h>    // for fopen, fread, etc.

/*
 * ==========================================================================
 * Metadata Ops
 * ==========================================================================
 */

bool file_exists(const char *path)
{
    if (!path) return false;
    FILE *f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool file_remove(const char *path)
{
    if (!path) return false;
    return remove(path) == 0;
}

/*
 * ==========================================================================
 * I/O Ops
 * ==========================================================================
 */

bool file_read_to_string(const char *path, string_t *out)
{
    if (!path || !out) return false;

    FILE *f = fopen(path, "rb");
    if (!f) return false;

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
    if (!path) return false;
    
    FILE *f = fopen(path, "wb"); // Overwrite, Binary
    if (!f) return false;

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
    if (!path) return false;

    FILE *f = fopen(path, "ab"); // Append, Binary
    if (!f) return false;

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
