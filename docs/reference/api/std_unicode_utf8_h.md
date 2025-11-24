# std/unicode/utf8.h

## `typedef struct {`


Result of a decoding operation.


---

## `[[nodiscard]] utf8_decode_result_t utf8_decode(const char *ptr, usize len);`


Decode the first UTF-8 character from a byte buffer.


- **`ptr`**: Pointer to the start of the utf8 sequence.
- **`len`**: Remaining length of the buffer.
- **Returns**: A result containing the codepoint and its byte width.


> **Note:** Safe and Lossy: If invalid UTF-8 is encountered, it returns
(0xFFFD, 1), allowing the caller to skip the bad byte and continue.


---

## `[[nodiscard]] usize utf8_encode(rune_t cp, char *buf);`


Encode a codepoint into a byte buffer.


- **`cp`**: The codepoint to encode.
- **`buf`**: Output buffer (must be at least 4 bytes).
- **Returns**: Number of bytes written (0 if cp is invalid, e.g. > 0x10FFFF).


---

## `static inline utf8_iter_t utf8_iter_new(str_t s) {`


Initialize iterator from a string slice.


---

## `[[nodiscard]] bool utf8_next(utf8_iter_t *it, rune_t *out_cp);`


Get the next codepoint.

- **`it`**: Iterator pointer.
- **`out_cp`**: [out] The decoded codepoint.
- **Returns**: true if a character was read, false if EOF.


---

## `[[nodiscard]] bool utf8_peek(utf8_iter_t *it, rune_t *out_cp);`


Peek the next codepoint without advancing.


---

