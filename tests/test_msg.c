#include <std/test.h>
#include <core/msg.h>
#include <core/type.h>

/*
 * ==========================================================================
 * Helper Structs for Dump
 * ==========================================================================
 */

typedef struct Point {
	int x;
	int y;
} point_t;

/*
 * ==========================================================================
 * Smoke Tests (Ensure no crashes on normal usage)
 * ==========================================================================
 */

TEST(msg_logging_smoke)
{
	/// these macros output to stderr
	/// we cannot easily capture stderr in C without complex pipe logic
	/// so we just ensure they don't crash the program with valid arguments

	silence({
		log_info("test info message: %d", 123);
		log_warn("test warn message");
		log_error("test error message");

		/// debug macro only works in debug builds
		/// but calling it should be safe regardless
		dbg("test debug message");
	});

	return true;
}

TEST(msg_dump_struct)
{
	/// test the __builtin_dump_struct wrapper
	point_t p = { .x = 10, .y = 20 };

	/// correct usage: pass a pointer
	silence({ dump(&p); });

	/// note: we cannot test 'dump(p)' here because it triggers
	/// a compile-time static_assert error, which stops the build.
	/// that is manual verification territory.

	return true;
}

/*
 * ==========================================================================
 * Death Tests (Safety Guarantees)
 * ==========================================================================
 */

TEST(msg_assertion_death)
{
	/// 1. simple asserrt
	/// verify that false condition triggers abort
	expect_panic(asserrt(false));

	/// verify that true condition proceeds
	asserrt(true);

	/// 2. message massert
	/// verify panic with formatting
	expect_panic(massert(1 == 2, "math is broken: %d != %d", 1, 2));

	/// verify success case
	massert(1 == 1, "math works");

	return true;
}

TEST(msg_panic_macros_death)
{
	/// 1. log_panic
	/// must abort unconditionally
	expect_panic(log_panic("fatal error test"));

	/// 2. todo
	/// must abort unconditionally
	expect_panic(todo("implement this feature later"));

	/// 3. unreach
	/// must abort unconditionally in debug builds
	expect_panic(unreach());

	return true;
}

/*
 * ==========================================================================
 * Side Effects Test
 * ==========================================================================
 */

TEST(msg_assertion_side_effects)
{
	/// verify that assertions evaluate their condition exactly once
	int counter = 0;

	/// condition is true (1 == 1), counter increments to 1
	asserrt(++counter == 1);
	expect_eq(counter, 1);

	/// massert condition is true, counter increments to 2
	massert(++counter == 2, "check side effect");
	expect_eq(counter, 2);

	return true;
}

int main()
{
	RUN(msg_logging_smoke);
	RUN(msg_dump_struct);
	RUN(msg_assertion_death);
	RUN(msg_panic_macros_death);
	RUN(msg_assertion_side_effects);

	SUMMARY();
}
