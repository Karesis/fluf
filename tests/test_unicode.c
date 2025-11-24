#include <std/test.h>
#include <std/unicode/utf8.h>
#include <string.h>

/*
 * ==========================================================================
 * Helpers
 * ==========================================================================
 */

#define EXPECT_RUNE(res, expected_val, expected_len)            \
	do {                                                    \
		expect_eq((res).value, (rune_t)(expected_val)); \
		expect_eq((res).len, (u8)(expected_len));       \
	} while (0)

#define REPLACEMENT UTF8_REPLACEMENT_CHARACTER /// 0xFFFD

/*
 * ==========================================================================
 * 1. Valid Decoding (Happy Path)
 * ==========================================================================
 */

TEST(utf8_valid_sequences)
{
	utf8_decode_result_t res;

	/// 1-byte: 'A' (U+0041)
	res = utf8_decode("A", 1);
	EXPECT_RUNE(res, 0x41, 1);

	/// 2-bytes: '¢' (Cent, U+00A2) -> C2 A2
	/// using string literal with hex escapes
	res = utf8_decode("\xC2\xA2", 2);
	EXPECT_RUNE(res, 0x00A2, 2);

	/// 3-bytes: '€' (Euro, U+20AC) -> E2 82 AC
	res = utf8_decode("\xE2\x82\xAC", 3);
	EXPECT_RUNE(res, 0x20AC, 3);

	/// 4-bytes: '😀' (Grinning Face, U+1F600) -> F0 9F 98 80
	res = utf8_decode("\xF0\x9F\x98\x80", 4);
	EXPECT_RUNE(res, 0x1F600, 4);

	return true;
}

/*
 * ==========================================================================
 * 2. Malformed Sequences (Security & Stability)
 * ==========================================================================
 */

TEST(utf8_truncated)
{
	utf8_decode_result_t res;

	/// 2-byte sequence missing last byte: "C2"
	res = utf8_decode("\xC2", 1);
	EXPECT_RUNE(res, REPLACEMENT, 1); /// consumes 1 bad byte

	/// 3-byte sequence missing last byte: "E2 82"
	res = utf8_decode("\xE2\x82", 2);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// 4-byte sequence missing last 2 bytes: "F0 9F"
	res = utf8_decode("\xF0\x9F", 2);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	return true;
}

TEST(utf8_invalid_bytes)
{
	utf8_decode_result_t res;

	/// unexpected continuation byte (0x80) at start
	res = utf8_decode("\x80", 1);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// invalid start byte (0xFF)
	res = utf8_decode("\xFF", 1);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// invalid start byte (0xF5) - above U+10FFFF
	res = utf8_decode("\xF5", 1);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	return true;
}

TEST(utf8_overlong)
{
	/// overlong encodings are ILLEGAL in UTF-8 (RFC 3629).
	/// they must be rejected, otherwise they can bypass security checks.

	utf8_decode_result_t res;

	/// aSCII 'A' (0x41) encoded as 2 bytes: C1 81
	/// 11000001 10000001 -> 00001 000001 -> 0x41
	res = utf8_decode("\xC1\x81", 2);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// slash '/' (0x2F) encoded as 2 bytes: C0 AF (Classic security hole)
	res = utf8_decode("\xC0\xAF", 2);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// u+20AC (Euro) encoded as 4 bytes (should be 3): F0 82 82 AC
	/// this is technically a 4-byte prefix E0.. should check specific ranges
	/// let's test the boundary for 3-byte overlong:
	/// u+07FF should be 2 bytes (DF BF).
	/// encoded as 3 bytes: E0 9F BF
	res = utf8_decode("\xE0\x9F\xBF", 3);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	return true;
}

TEST(utf8_surrogates)
{
	/// uTF-16 Surrogates (U+D800 - U+DFFF) are forbidden in UTF-8.
	utf8_decode_result_t res;

	/// u+D800 encoded as 3 bytes: ED A0 80
	res = utf8_decode("\xED\xA0\x80", 3);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// u+DFFF encoded as 3 bytes: ED BF BF
	res = utf8_decode("\xED\xBF\xBF", 3);
	EXPECT_RUNE(res, REPLACEMENT, 1);

	/// valid character just before surrogates: U+D7FF (ED 9F BF)
	res = utf8_decode("\xED\x9F\xBF", 3);
	EXPECT_RUNE(res, 0xD7FF, 3); /// should pass

	/// valid character just after surrogates: U+E000 (EE 80 80)
	res = utf8_decode("\xEE\x80\x80", 3);
	EXPECT_RUNE(res, 0xE000, 3); /// should pass

	return true;
}

/*
 * ==========================================================================
 * 3. Encoding
 * ==========================================================================
 */

TEST(utf8_encoding)
{
	char buf[5] = { 0 };
	usize len;

	/// 1. aSCII
	len = utf8_encode(0x41, buf); /// 'A'
	expect_eq(len, usize_(1));
	expect((u8)buf[0] == 0x41);

	/// 2. euro (3 bytes)
	len = utf8_encode(0x20AC, buf);
	expect_eq(len, usize_(3));
	/// e2 82 AC
	expect((u8)buf[0] == 0xE2);
	expect((u8)buf[1] == 0x82);
	expect((u8)buf[2] == 0xAC);

	/// 3. invalid: Surrogate (U+D800)
	/// should fallback to replacement char (EF BF BD)
	len = utf8_encode(0xD800, buf);
	expect_eq(len, usize_(3));
	expect((u8)buf[0] == 0xEF); /// u+FFFD byte 1
	expect((u8)buf[1] == 0xBF); /// u+FFFD byte 2
	expect((u8)buf[2] == 0xBD); /// u+FFFD byte 3

	return true;
}

/*
 * ==========================================================================
 * 4. Iterator
 * ==========================================================================
 */

TEST(utf8_iterator)
{
	/// "A€" -> 1 byte + 3 bytes
	/// [0x41, 0xE2, 0x82, 0xAC]
	str_t s = str("A\xE2\x82\xAC");

	utf8_iter_t it = utf8_iter_new(s);
	rune_t cp;

	/// first: 'A'
	expect(utf8_next(&it, &cp));
	expect_eq(cp, rune(0x41));
	expect_eq(it.cursor, usize_(1));

	/// peek: '€'
	expect(utf8_peek(&it, &cp));
	expect_eq(cp, rune(0x20AC));
	expect_eq(it.cursor, usize_(1)); /// peek shouldn't move

	/// second: '€'
	expect(utf8_next(&it, &cp));
	expect_eq(cp, rune(0x20AC));
	expect_eq(it.cursor, usize_(4)); /// 1 + 3

	/// eOF
	expect(!utf8_next(&it, &cp));

	return true;
}

int main()
{
	RUN(utf8_valid_sequences);
	RUN(utf8_truncated);
	RUN(utf8_invalid_bytes);
	RUN(utf8_overlong);
	RUN(utf8_surrogates);
	RUN(utf8_encoding);
	RUN(utf8_iterator);

	SUMMARY();
}
