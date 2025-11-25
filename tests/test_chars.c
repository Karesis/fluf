#include <std/test.h>
#include <std/strings/chars.h>

TEST(char_properties)
{
	expect(char_is_digit('0'));
	expect(!char_is_digit('a'));

	expect(char_is_space(' '));
	expect(char_is_space('\n'));
	expect(!char_is_space('A'));

	return true;
}

int main()
{
	RUN(char_properties);
	SUMMARY();
}
