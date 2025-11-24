#include <std/unicode/prop.h>
#include <core/type.h>
#include <core/macros.h>

#include "tables.gen"

/*
 * ==========================================================================
 * Internal Helper: Binary Search
 * ==========================================================================
 */

/**
 * @brief Check if a codepoint is within any range in the sorted table.
 * Complexity: O(log N)
 */
static bool _is_in_table(rune_t c, const unicode_range_t *table, usize count)
{
    if (count == 0) return false;

    // Optimization: Check bounds first
    if (c < table[0].start || c > table[count - 1].end) {
        return false;
    }

    usize low = 0;
    usize high = count;

    while (low < high) {
        usize mid = low + (high - low) / 2;
        const unicode_range_t *range = &table[mid];

        if (c < range->start) {
            high = mid;
        } else if (c > range->end) {
            low = mid + 1;
        } else {
            // c >= start && c <= end -> Match!
            return true;
        }
    }
    return false;
}

/*
 * ==========================================================================
 * Public API Implementation
 * ==========================================================================
 */

bool unicode_is_whitespace(rune_t c)
{
    // Fast path for ASCII whitespace
    if (c <= 0x7F) {
        return c == ' ' || (c >= 0x09 && c <= 0x0D);
    }
    return _is_in_table(c, WHITE_SPACE_TABLE, array_size(WHITE_SPACE_TABLE));
}

bool unicode_is_xid_start(rune_t c)
{
    // Fast path for ASCII (a-z, A-Z)
    // Note: '_' is NOT XID_Start in pure Unicode, but many langs allow it.
    // Rust allows '_' in identifiers but checks XID properties.
    // Pure XID_Start does not include '_'. 
    if (c <= 0x7F) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    return _is_in_table(c, XID_START_TABLE, array_size(XID_START_TABLE));
}

bool unicode_is_xid_continue(rune_t c)
{
    // Fast path for ASCII (a-z, A-Z, 0-9, _)
    if (c <= 0x7F) {
        return (c >= 'a' && c <= 'z') || 
               (c >= 'A' && c <= 'Z') || 
               (c >= '0' && c <= '9') || 
               c == '_';
    }
    return _is_in_table(c, XID_CONTINUE_TABLE, array_size(XID_CONTINUE_TABLE));
}

bool unicode_is_numeric(rune_t c)
{
    if (c <= 0x7F) {
        return c >= '0' && c <= '9';
    }
    // For now we don't generate a numeric table (it's huge and often not needed for compilers).
    // Compilers usually strictly define digits as 0-9.
    // If you need full unicode digits (like '１'), you can add "General_Category=Nd" to python script.
    return false;
}
