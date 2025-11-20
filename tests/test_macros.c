#include <std/test.h>
#include <core/macros.h>
#include <core/type.h>

/*
 * ==========================================================================
 * Helper Structures & Types
 * ==========================================================================
 */

typedef int my_int;

struct Point {
	int x;
	int y;
};

struct Node {
	struct Node *next;
	int value;
};

struct Container {
	int id;
	char name[10];
	struct Node node;
};

typedef struct {
	char data[10];
} TypedefStruct;

union Variant {
	int i;
	float f;
};

enum Color { RED, GREEN, BLUE };

/*
 * ==========================================================================
 * Type Traits Tests
 * ==========================================================================
 */

TEST(traits_pointer_vs_array)
{
	/// setup variables
	int x = 10;
	int *ptr = &x;
	int arr[5] = { 0 };
	void *vptr = &x;

	/// [positive] check standard pointers
	expect(is_pointer(ptr));
	expect(is_pointer(vptr));

	/// check pointer to pointer
	int **pptr = &ptr;
	expect(is_pointer(pptr));

	/// [negative] check non-pointers
	expect(is_pointer(x) == false);
	expect(is_pointer(123) == false);

	/// [crucial] check array behavior
	/// an array variable itself is NOT a pointer in type system
	expect(is_array(arr));
	expect(is_pointer(arr) == false);

	/// check array decay behavior
	/// assigning array to pointer causes decay
	int *decayed = arr;
	expect(is_pointer(decayed));
	expect(is_array(decayed) == false);

	return true;
}

TEST(traits_string_literals)
{
	/// [crucial] string literal is an array (char[])
	/// usually const char[N]
	expect(is_array("hello"));
	expect(is_pointer("hello") == false);

	/// verify size is N+1 (including null terminator)
	expect_eq(sizeof("hello"), usize(6));

	/// check decayed string pointer
	const char *str_ptr = "hello";
	expect(is_pointer(str_ptr));
	expect(is_array(str_ptr) == false);

	return true;
}

TEST(traits_numeric)
{
	/// [positive] check integer types
	expect(is_integer(1));
	expect(is_integer(u32(100)));
	expect(is_integer((char)'a'));
	expect(is_integer(true));

	/// check typedefs
	my_int m = 10;
	volatile int v = 20;
	expect(is_integer(m));
	expect(is_integer(v));

	/// check enums (treated as integers)
	enum Color c = RED;
	expect(is_integer(c));

	/// [positive] check floating point types
	expect(is_floating(1.0f));
	expect(is_floating(1.0));
	expect(is_floating(1e-10));

	/// [negative] cross check
	expect(is_integer(1.5) == false);
	expect(is_floating(1) == false);
	expect(is_integer("str") == false);

	return true;
}

TEST(traits_composites)
{
	struct Point p = { 1, 2 };
	TypedefStruct ts;
	union Variant u;

	/// check struct detection
	expect(is_struct(p));
	expect(is_struct(ts));

	/// check union detection
	expect(is_union(u));

	/// check mutual exclusion
	/// a union is not a struct in gcc/clang classification
	expect(is_struct(u) == false);
	expect(is_union(p) == false);

	/// check pointers to composites
	/// pointers are always _TYPE_POINTER, not RECORD/UNION
	expect(is_struct(&p) == false);
	expect(is_union(&u) == false);

	return true;
}

TEST(traits_identity_check)
{
	int a = 1;
	long b = 2;
	int c = 3;

	/// check is_type
	expect(is_type(a, int));
	expect(is_type(b, long));
	expect(is_type(a, long) == false);

	/// check typecmp
	expect(typecmp(a, c)); /// int vs int
	expect(typecmp(a, b) == false); /// int vs long

	/// check const/volatile ignoring behavior
	/// __builtin_types_compatible_p ignores top-level qualifiers
	const int d = 4;
	expect(typecmp(a, d));

	return true;
}

/*
 * ==========================================================================
 * Utility Macros Tests
 * ==========================================================================
 */

TEST(utils_container_of)
{
	/// setup data
	struct Container c;
	c.id = 999;
	c.node.value = 42;

	/// simulate getting a pointer to a member
	struct Node *node_ptr = &c.node;

	/// perform magic recovery
	struct Container *recovered =
		container_of(node_ptr, struct Container, node);

	/// check address equality
	expect(recovered == &c);

	/// check data integrity
	expect_eq(recovered->id, 999);
	expect_eq(recovered->node.value, 42);

	return true;
}

TEST(utils_array_size)
{
	int a[10];
	char b[3];
	struct Point c[5];

	/// check basic array sizes
	expect_eq(array_size(a), usize(10));
	expect_eq(array_size(b), usize(3));
	expect_eq(array_size(c), usize(5));

	/// check multi-dimensional array
	/// should return the size of the first dimension
	int grid[4][5];
	expect_eq(array_size(grid), usize(4));
	expect_eq(array_size(grid[0]), usize(5));

	/// note: we cannot test array_size(ptr) failure here
	/// because it triggers a compile-time static_assert error

	return true;
}

/*
 * ==========================================================================
 * Compiler Hints Tests (Smoke Tests)
 * ==========================================================================
 */

/// helper function to test attribute syntax
static void helper_unused_attr(int x uattr)
{
	unused(x); /// suppress warning
}

TEST(hints_compilation_smoke)
{
	int a = 1;
	int b = 0;

	/// check likely/unlikely logic preservation
	expect(likely(a == 1));
	expect(unlikely(b == 1) == false);

	/// check control flow syntax
	if (likely(a)) {
		DO_NOTHING; /// check do nothing syntax
	} else {
		expect(false); /// should not reach here
	}

	/// check unused macros compilation
	helper_unused_attr(42);

	return true;
}

int main()
{
	RUN(traits_pointer_vs_array);
	RUN(traits_string_literals);
	RUN(traits_numeric);
	RUN(traits_composites);
	RUN(traits_identity_check);
	RUN(utils_container_of);
	RUN(utils_array_size);
	RUN(hints_compilation_smoke);

	SUMMARY();
}
