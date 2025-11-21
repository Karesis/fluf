#include <std/test.h>
#include <std/allocers/system.h>
#include <core/math.h> /// is_aligned

TEST(sys_alloc_basic)
{
	allocer_t sys = allocer_system();

	/// 1. basic Alloc
	int *p = (int *)allocer_alloc(sys, layout_of(int));
	expect(p != nullptr);
	*p = 100;
	allocer_free(sys, p, layout_of(int));

	return true;
}

TEST(sys_alloc_alignment)
{
	allocer_t sys = allocer_system();

	/// 2. high Alignment (Page size)
	layout_t page_layout = layout(1024, 4096);
	void *p_page = allocer_alloc(sys, page_layout);

	expect(p_page != nullptr);
	/// verify the pointer address is divisible by 4096
	expect(is_aligned((uptr)p_page, 4096));

	allocer_free(sys, p_page, page_layout);

	return true;
}

TEST(sys_realloc_logic)
{
	allocer_t sys = allocer_system();

	/// alloc small
	layout_t l1 = layout_of_array(int, 10);
	int *arr = (int *)allocer_alloc(sys, l1);
	for (int i = 0; i < 10; ++i)
		arr[i] = i;

	/// realloc large (and maybe change alignment)
	layout_t l2 = layout_of_array(int, 20); /// larger
	/// force higher alignment to stress test the logic
	l2.align = 64;

	int *new_arr = (int *)allocer_realloc(sys, arr, l1, l2);
	expect(new_arr != nullptr);
	expect(is_aligned((uptr)new_arr, 64)); /// check align change

	/// verify data preserved
	for (int i = 0; i < 10; ++i)
		expect_eq(new_arr[i], i);

	allocer_free(sys, new_arr, l2);
	return true;
}

int main()
{
	RUN(sys_alloc_basic);
	RUN(sys_alloc_alignment);
	RUN(sys_realloc_logic);
	SUMMARY();
}
