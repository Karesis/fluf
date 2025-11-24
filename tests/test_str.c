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

#include <std/test.h>
#include <std/strings/str.h>

/*
 * ==========================================================================
 * Basic Construction & Inspection
 * ==========================================================================
 */

TEST(str_construction)
{
    /// literal construction
    str_t s1 = str("hello");
    expect_eq(s1.len, usize_(5));
    expect(s1.ptr[0] == 'h');

    /// from cstr
    const char *raw = "world";
    str_t s2 = str_from_cstr(raw);
    expect_eq(s2.len, usize_(5));
    expect(s2.ptr == raw);

    /// empty case
    str_t s3 = str_from_cstr(nullptr);
    expect_eq(s3.len, usize_(0));
    expect(s3.ptr == nullptr);

    /// manual parts construction (with safety check)
    str_t s4 = str_from_parts("test", 4);
    expect_eq(s4.len, usize_(4));
    
    /// check panic on invalid input
    /// str_from_parts(NULL, 5) should panic
    expect_panic(str_from_parts(nullptr, 5));

    return true;
}

TEST(str_equality)
{
    str_t s1 = str("foo");
    str_t s2 = str("foo");
    str_t s3 = str("bar");
    str_t empty = str("");

    /// check slice vs slice
    expect(str_eq(s1, s2));
    expect(!str_eq(s1, s3));
    expect(str_eq(empty, str_from_cstr("")));

    /// check slice vs cstr
    expect(str_eq_cstr(s1, "foo"));
    expect(!str_eq_cstr(s1, "fo"));
    expect(!str_eq_cstr(s1, "fool"));

    return true;
}

TEST(str_checks)
{
    str_t s = str("hello world");
    
    /// starts_with
    expect(str_starts_with(s, str("hello")));
    expect(!str_starts_with(s, str("world")));
    expect(str_starts_with(s, str(""))); /// empty prefix is always true
    
    /// ends_with
    expect(str_ends_with(s, str("world")));
    expect(!str_ends_with(s, str("hello")));
    expect(str_ends_with(s, str(""))); /// empty suffix is always true

    return true;
}

/*
 * ==========================================================================
 * Manipulation Tests
 * ==========================================================================
 */

TEST(str_trimming)
{
    str_t s = str("  hello  ");
    
    /// trim left
    str_t left = str_trim_left(s);
    expect_eq(left.len, usize_(7));
    expect(str_eq(left, str("hello  ")));

    /// trim right
    str_t right = str_trim_right(s);
    expect_eq(right.len, usize_(7));
    expect(str_eq(right, str("  hello")));

    /// trim both
    str_t both = str_trim(s);
    expect_eq(both.len, usize_(5));
    expect(str_eq(both, str("hello")));

    /// trim empty/all-whitespace
    str_t spaces = str("   ");
    expect(str_is_empty(str_trim(spaces)));

    return true;
}

TEST(str_splitting)
{
    str_t input = str("a,b,c");
    str_t chunk;
    int count = 0;

    /// 1st: "a"
    expect(str_split(&input, ',', &chunk));
    expect(str_eq(chunk, str("a")));
    count++;

    /// 2nd: "b"
    expect(str_split(&input, ',', &chunk));
    expect(str_eq(chunk, str("b")));
    count++;

    /// 3rd: "c"
    expect(str_split(&input, ',', &chunk));
    expect(str_eq(chunk, str("c")));
    count++;

    /// End (should be false)
    expect(str_split(&input, ',', &chunk) == false);
    
    expect_eq(count, 3);

    return true;
}

/*
 * ==========================================================================
 * Iterator Macros Tests
 * ==========================================================================
 */

TEST(str_iterators)
{
    str_t csv = str("apple,banana,cherry");

    /// 1. Test Split Iterator (str_for_split)
    int count = 0;
    str_for_split(fruit, csv, ',') {
        if (count == 0) expect(str_eq_cstr(fruit, "apple"));
        if (count == 1) expect(str_eq_cstr(fruit, "banana"));
        if (count == 2) expect(str_eq_cstr(fruit, "cherry"));
        count++;
    }
    expect_eq(count, 3);

    /// Verify original slice is untouched (Non-destructive)
    expect_eq(csv.len, usize_(19));
    expect(csv.ptr[0] == 'a');

    /// 2. Test Lines Iterator (str_for_lines)
    str_t text = str("line1\nline2\n");
    int lines = 0;
    str_for_lines(l, text) {
        if (lines == 0) expect(str_eq_cstr(l, "line1"));
        /// Note: str_for_lines uses the smart split logic, so it swallows the last empty line
        if (lines == 1) expect(str_eq_cstr(l, "line2"));
        lines++;
    }
    expect(lines >= 2);

    /// 3. Test Char Iterator (str_for_each)
    str_t word = str("hi");
    int chars = 0;
    str_for_each(c, word) {
        if (chars == 0) expect(c == 'h');
        if (chars == 1) expect(c == 'i');
        chars++;
    }
    expect_eq(chars, 2);

    return true;
}

TEST(str_lines_iterator_smart)
{
    /// Case 1: Standard Unix text with trailing newline
    str_t unix_text = str("A\nB\n");
    int count = 0;

    str_for_lines(line, unix_text) {
        if (count == 0) expect(str_eq_cstr(line, "A"));
        if (count == 1) expect(str_eq_cstr(line, "B"));
        count++;
    }
    /// Crucial: Should be 2, not 3. The trailing empty line is ignored.
    expect_eq(count, 2);

    /// Case 2: Windows text (\r\n)
    str_t win_text = str("Hello\r\nWorld");
    count = 0;

    str_for_lines(line, win_text) {
        if (count == 0) {
            expect(str_eq_cstr(line, "Hello"));
            /// Verify \r is stripped
            expect(line.len == usize_(5));
        }
        if (count == 1) expect(str_eq_cstr(line, "World"));
        count++;
    }
    expect_eq(count, 2);

    /// Case 3: No newline at all
    str_t raw = str("SingleLine");
    count = 0;
    str_for_lines(line, raw) {
        expect(str_eq_cstr(line, "SingleLine"));
        count++;
    }
    expect_eq(count, 1);

    return true;
}

int main()
{
    RUN(str_construction);
    RUN(str_equality);
    RUN(str_checks);
    RUN(str_trimming);
    RUN(str_splitting);
    RUN(str_iterators);
    RUN(str_lines_iterator_smart);
    
    SUMMARY();
}
