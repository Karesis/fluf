#include <std/strings/str.h>
#include <std/strings/chars.h>
#include <core/math.h> /// for checked_mul/add

ResU64 str_parse_u64(str_t s)
{
	if (str_is_empty(s))
		return (ResU64)err("Empty string");

	u64 res = 0;
	for (usize i = 0; i < s.len; ++i) {
		char c = s.ptr[i];

		if (!char_is_digit(c)) {
			return (ResU64)err("Invalid character in number");
		}

		u64 digit = (u64)(c - '0');

		/// res = res * 10 + digit
		/// check Overflow: Mul
		if (checked_mul(res, 10, &res))
			return (ResU64)err("Overflow");
		/// check Overflow: Add
		if (checked_add(res, digit, &res))
			return (ResU64)err("Overflow");
	}

	return (ResU64)ok(res);
}

ResI64 str_parse_i64(str_t s)
{
	if (str_is_empty(s))
		return (ResI64)err("Empty string");

	bool negative = false;
	str_t num_part = s;

	if (s.ptr[0] == '-') {
		negative = true;
		/// advance pointer by 1
		if (s.len == 1)
			return (ResI64)err("Invalid number (just '-')");
		num_part = str_from_parts(s.ptr + 1, s.len - 1);
	} else if (s.ptr[0] == '+') {
		if (s.len == 1)
			return (ResI64)err("Invalid number (just '+')");
		num_part = str_from_parts(s.ptr + 1, s.len - 1);
	}

	/// reuse u64 parser for the magnitude
	ResU64 res_u = str_parse_u64(num_part);
	if (!res_u.ok) {
		return (ResI64)err(res_u.err);
	}

	u64 val = res_u.val;

	/// check i64 bounds
	/// max i64 = 9223372036854775807
	/// min i64 = -9223372036854775808 (absolute is 9223372036854775808)

	if (negative) {
		/// abs(INT64_MIN) is 1 larger than INT64_MAX
		if (val > (u64)INT64_MAX + 1)
			return (ResI64)err("Underflow");
		return (ResI64)ok(-(i64)val);
	} else {
		if (val > (u64)INT64_MAX)
			return (ResI64)err("Overflow");
		return (ResI64)ok((i64)val);
	}
}
