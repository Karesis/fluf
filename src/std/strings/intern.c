#include <std/strings/intern.h>
#include <core/hash.h>
#include <string.h>

/*
 * ==========================================================================
 * 1. Map Operations for str_t
 * ==========================================================================
 */

static u64 _hash_str(const void *key)
{
	const str_t *s = (const str_t *)key;
	return hash_bytes(s->ptr, s->len);
}

static bool _eq_str(const void *a, const void *b)
{
	const str_t *s1 = (const str_t *)a;
	const str_t *s2 = (const str_t *)b;
	return str_eq(*s1, *s2);
}

static const map_ops_t MAP_OPS_STR = { .hash = _hash_str, .equals = _eq_str };

/*
 * ==========================================================================
 * 2. Lifecycle
 * ==========================================================================
 */

bool intern_init(interner_t *it, allocer_t alc)
{
	bump_init(&it->pool, alc, 1);

	if (!map_init(it->map, alc, MAP_OPS_STR)) {
		bump_deinit(&it->pool);
		return false;
	}

	if (!vec_init(it->vec, alc, 64)) {
		map_deinit(it->map);
		bump_deinit(&it->pool);
		return false;
	}

	return true;
}

void intern_deinit(interner_t *it)
{
	map_deinit(it->map);
	vec_deinit(it->vec);
	bump_deinit(&it->pool);
}

interner_t *intern_new(allocer_t alc)
{
	layout_t l = layout_of(interner_t);
	interner_t *it = (interner_t *)allocer_alloc(alc, l);

	if (it) {
		if (!intern_init(it, alc)) {
			allocer_free(alc, it, l);
			return nullptr;
		}
	}
	return it;
}

void intern_drop(interner_t *it)
{
	if (it) {
		allocer_t alc = it->pool.backing;
		intern_deinit(it);
		allocer_free(alc, it, layout_of(interner_t));
	}
}

/*
 * ==========================================================================
 * 3. Core Logic
 * ==========================================================================
 */

symbol_t intern(interner_t *it, str_t s)
{
	/// 1. fast Lookup
	symbol_t *existing = map_get(it->map, s);
	if (existing) {
		return *existing;
	}

	/// 2. slow Path: Intern New String

	/// a. Alloc stable memory
	char *stable_ptr = bump_dup_str(&it->pool, s);
	if (unlikely(!stable_ptr)) {
		log_panic("Interner pool OOM");
	}

	/// b. Create Key
	str_t stable_str = str_from_parts(stable_ptr, s.len);

	/// c. Create Symbol
	symbol_t sym = { .id = (u32)vec_len(it->vec) };

	/// d. Store (Vec + Map)
	/// order matters: Map stores the key pointing to Bump memory
	if (unlikely(!vec_push(it->vec, stable_str))) {
		log_panic("Interner vec OOM");
	}

	if (unlikely(!map_put(it->map, stable_str, sym))) {
		log_panic("Interner map OOM");
	}

	return sym;
}

str_t intern_resolve(const interner_t *it, symbol_t sym)
{
	/// safe because vec_at performs bounds checking
	return vec_at(it->vec, sym.id);
}

const char *intern_resolve_cstr(const interner_t *it, symbol_t sym)
{
	str_t s = intern_resolve(it, sym);
	/// safe because we manually appended \0 during intern()
	return s.ptr;
}

usize intern_count(const interner_t *it)
{
	return vec_len(it->vec);
}
