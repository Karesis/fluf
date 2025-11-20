#pragma once

/// for log_panic
#include <core/msg.h>
/// for likely, unlikely
#include <core/macros.h>

/// for bool
#include <stdbool.h>

/*
 * ============================================================================
 * 1. Data Structures 
 * ============================================================================
 * Design Philosophy:
 * Option and Result share a "Duck Typing" layout for the success path.
 * Both have `.ok` and `.val`. Compile-time reflection is achieved via
 * `_tag` size.
 */

#define _TAG_SIZE_OPTION 1
#define _TAG_SIZE_RESULT 2

/**
 * @brief Define the body of an Option(T) struct.
 *
 * Includes a dummy `err` field to satisfy duck-typing macros (like try/unwrap).
 * The `err` field is never read at runtime for Options.
 *
 * @note Tag size `_TAG_SIZE_OPTION` (1) indicates Option.
 */
#define option(T) \
	bool ok;  \
	T val;    \
	char err; \
	char _tag[_TAG_SIZE_OPTION]

#define defOption(T, name)    \
	typedef struct name { \
		option(T);    \
	} name

/**
 * @brief Define the body of a Result(T, E) struct.
 *
 * @note Tag size `_TAG_SIZE_RESULT` (2) indicates Result.
 */
#define result(T, E)   \
	bool ok;       \
	union {        \
		T val; \
		E err; \
	};             \
	char _tag[_TAG_SIZE_RESULT]

#define defResult(T, E, name) \
	typedef struct name { \
		result(T, E); \
	} name

/*
 * ============================================================================
 * 2. Constructors
 * ============================================================================
 */

#define some(v) { .ok = true, .val = (v) }
#define none { .ok = false }

#define ok(v) { .ok = true, .val = (v) }
#define err(e) { .ok = false, .err = (e) }

/*
 * ============================================================================
 * 3. Internal Helpers 
 * ============================================================================
 */

/**
 * @brief Check if the container is a Result type at compile time.
 */
#define _is_result(x) (sizeof((x)._tag) == 2)

/*
 * ============================================================================
 * 4. Unwrap Mechanisms
 * ============================================================================
 */

/**
 * @brief Safe unwrap. Panics if value is not present.
 *
 * ### Smart Logic:
 * 1. If Option: Panic with "Option is None".
 * 2. If Result: Panic with "Result is Err: <value>", automatically using
 * the correct format specifier (e.g., %d for int, %s for string) via fmt().
 */
#define unwrap(x)                                                             \
	({                                                                    \
		auto _res = (x);                                              \
		if (unlikely(!_res.ok)) {                                     \
			if (_is_result(_res)) {                               \
				/* 1. Print Header */                         \
				fprintf(stderr,                               \
					"[%s] [%s:%d] %s(): unwrap failed: ", \
					_LOG_LEVEL_PANIC, __FILE__, __LINE__, \
					__func__);                            \
				/* 2. Print Value using Generic fmt() */      \
				fprintf(stderr, fmt(_res.err), _res.err);     \
				/* 3. Print Newline and Abort */              \
				fprintf(stderr, "\n");                        \
				abort();                                      \
			} else {                                              \
				log_panic("unwrap failed: Option is None");   \
			}                                                     \
		}                                                             \
		_res.val;                                                     \
	})

/**
 * @brief Check the value to be present, with context.
 *
 * @param msg Context message (e.g. "Failed to load config").
 *
 * ### Logic:
 * Prints: "[PANIC] ... <msg>: <error_value>"
 * It works for ANY error type supported by fmt() (int, char*, etc.),
 * not just strings!
 */
#define check(x, msg)                                                              \
	({                                                                         \
		auto _res = (x);                                                   \
		if (unlikely(!_res.ok)) {                                          \
			if (_is_result(_res)) {                                    \
				/* Manual expansion to support generic printing */ \
				fprintf(stderr, "[%s] [%s:%d] %s(): %s: ",         \
					_LOG_LEVEL_PANIC, __FILE__, __LINE__,      \
					__func__, msg);                            \
				fprintf(stderr, fmt(_res.err), _res.err);          \
				fprintf(stderr, "\n");                             \
				abort();                                           \
			} else {                                                   \
				log_panic("%s: Option is None", msg);              \
			}                                                          \
		}                                                                  \
		_res.val;                                                          \
	})

/**
 * @brief Return the inner value or a default value if None/Err.
 *
 * @param x   The Option/Result container.
 * @param def The default value to return if x is not ok.
 */
#define unwrap_or(x, def)                   \
	({                                  \
		auto _res = (x);            \
		_res.ok ? _res.val : (def); \
	})

/*
 * ============================================================================
 * 5. Control Flow & Utilities
 * ============================================================================
 */

#define is_ok(x) ((x).ok)
#define is_err(x) (!((x).ok))
#define is_some(x) ((x).ok)
#define is_none(x) (!((x).ok))

/**
 * @brief "If Let" syntax sugar.
 *
 * Usage:
 * if_let(val, my_option) {
 * printf("Got %d", val);
 * }
 */
#define if_let(var_name, expr)                                  \
	auto _container_##var_name = (expr);                    \
	if (_container_##var_name.ok)                           \
		for (auto var_name = _container_##var_name.val; \
		     _container_##var_name.ok;                  \
		     _container_##var_name.ok = false)

/**
 * @brief Propagate errors (The '?' operator in Rust).
 *
 * Logic:
 * - If Ok/Some: Evaluate to the inner value.
 * - If Err/None: RETURN from the current function immediately.
 *
 * Handling:
 * - Result: Returns `Err(original_err)`.
 * - Option: Returns `None` (zero-initializes the return struct).
 *
 * @warning The return type of the current function must match the container type.
 */
#define try(expr)                                                         \
	({                                                                \
		auto _try_res = (expr);                                   \
		if (unlikely(!_try_res.ok)) {                             \
			if (_is_result(_try_res)) {                       \
				return (typeof(_try_res)){                \
					.ok = false, .err = _try_res.err  \
				};                                        \
			} else {                                          \
				return (typeof(_try_res)){ .ok = false }; \
			}                                                 \
		}                                                         \
		_try_res.val;                                             \
	})
