#include <std/test.h>
#include <std/fs.h>
#include <std/allocers/system.h>
#include <string.h>

#define TEST_FILE "test_fs_sandbox.bin"
#define NON_EXISTENT_FILE "test_fs_ghost.bin"

/*
 * ==========================================================================
 * Helpers
 * ==========================================================================
 */

static void clean_env() {
    if (file_exists(TEST_FILE)) file_remove(TEST_FILE);
    if (file_exists(NON_EXISTENT_FILE)) file_remove(NON_EXISTENT_FILE);
}

/*
 * ==========================================================================
 * 1. Lifecycle & Metadata
 * ==========================================================================
 */

TEST(fs_lifecycle)
{
    clean_env();

    /// 1. Check non-existent
    expect(file_exists(NON_EXISTENT_FILE) == false);

    /// 2. Create via write
    expect(file_write(TEST_FILE, str("touch")));
    expect(file_exists(TEST_FILE) == true);

    /// 3. Remove
    expect(file_remove(TEST_FILE) == true);
    expect(file_exists(TEST_FILE) == false);

    /// 4. Remove non-existent (should fail or return false, depending on API design)
    /// Your API returns false if path doesn't exist.
    expect(file_remove(NON_EXISTENT_FILE) == false);

    return true;
}

/*
 * ==========================================================================
 * 2. Read / Write Logic
 * ==========================================================================
 */

TEST(fs_overwrite_behavior)
{
    clean_env();
    allocer_t sys = allocer_system();
    string_t s;
    expect(string_init(&s, sys, 0));

    /// 1. Initial Write
    expect(file_write(TEST_FILE, str("Version 1")));
    
    /// 2. Overwrite
    /// file_write should truncate the file properly ("wb" mode)
    expect(file_write(TEST_FILE, str("V2")));

    /// 3. Verify
    expect(file_read_to_string(TEST_FILE, &s));
    expect(str_eq(string_as_str(&s), str("V2")));
    
    string_deinit(&s);
    clean_env();
    return true;
}

TEST(fs_append_logic)
{
    clean_env();
    allocer_t sys = allocer_system();
    string_t s;
    expect(string_init(&s, sys, 0));

    /// 1. Append to NON-EXISTENT file
    /// Standard C "ab" mode creates the file if missing.
    expect(file_append(TEST_FILE, str("Start")));
    expect(file_exists(TEST_FILE));

    /// 2. Append to existing
    expect(file_append(TEST_FILE, str("End")));

    /// 3. Verify
    expect(file_read_to_string(TEST_FILE, &s));
    expect(str_eq(string_as_str(&s), str("StartEnd")));

    string_deinit(&s);
    clean_env();
    return true;
}

/*
 * ==========================================================================
 * 3. Advanced / Edge Cases
 * ==========================================================================
 */

TEST(fs_read_into_existing_buffer)
{
    clean_env();
    allocer_t sys = allocer_system();
    
    /// Create dummy file
    expect(file_write(TEST_FILE, str("World")));

    /// Init string with existing data
    string_t s;
    expect(string_init(&s, sys, 0));
    expect(string_append_cstr(&s, "Hello "));

    /// Read file SHOULD APPEND to the buffer, not overwrite it
    expect(file_read_to_string(TEST_FILE, &s));

    /// Verify concatenation
    expect(str_eq(string_as_str(&s), str("Hello World")));

    string_deinit(&s);
    clean_env();
    return true;
}

TEST(fs_binary_safety)
{
    clean_env();
    allocer_t sys = allocer_system();

    /// Construct data with embedded NULL bytes
    /// "H\0\0W" (len 4)
    char raw[] = {'H', '\0', '\0', 'W'};
    str_t bin_data = str_from_parts(raw, 4);

    /// 1. Write Binary
    expect(file_write(TEST_FILE, bin_data));

    /// 2. Read Binary
    string_t s;
    expect(string_init(&s, sys, 0));
    expect(file_read_to_string(TEST_FILE, &s));

    /// 3. Verify Size and Content
    expect_eq(string_len(&s), usize_(4));
    expect(s.data[0] == 'H');
    expect(s.data[1] == '\0');
    expect(s.data[2] == '\0');
    expect(s.data[3] == 'W');

    string_deinit(&s);
    clean_env();
    return true;
}

TEST(fs_fail_conditions)
{
    clean_env();
    allocer_t sys = allocer_system();
    string_t s;
    expect(string_init(&s, sys, 0));

    /// 1. Read non-existent file
    expect(file_read_to_string(NON_EXISTENT_FILE, &s) == false);
    
    /// 2. Buffer should remain untouched
    expect(string_len(&s) == 0);

    /// 3. Write to invalid path (e.g., directory that doesn't exist)
    /// Note: Linux specific path, might fail differently on Windows but generally fails.
    /// Using a path with '/' at end usually implies directory which fails for fopen write.
    expect(file_write("non_existent_dir/file.txt", str("fail")) == false);

    string_deinit(&s);
    return true;
}

int main()
{
    RUN(fs_lifecycle);
    RUN(fs_overwrite_behavior);
    RUN(fs_append_logic);
    RUN(fs_read_into_existing_buffer);
    RUN(fs_binary_safety);
    RUN(fs_fail_conditions);
    
    clean_env(); // Final cleanup
    SUMMARY();
}
