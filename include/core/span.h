/*
 *    Copyright 2025 Karesis
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

/// for is_type, typecmp
#include <core/macros.h>
/// for usize
#include <core/type.h>

/// for bool
#include <stdbool.h>

/**
 * @brief [start, end)
 */
struct Span {
	usize start;
	usize end;
};

typedef struct Span span_t;

/**
 * @brief Get a struct Span.
 *
 * @note if start > end, it will return a Struct Span 
 * with start == end (empty span)
 */
static inline span_t span(usize start, usize end)
{
	return (span_t){ .start = start, .end = (start > end ? start : end) };
}

/**
 * @brief Get span from [start, start + len)
 */
static inline span_t span_from_len(usize start, usize len)
{
	return (span_t){ .start = start, .end = start + len };
}

/**
 * @brief Compare if two span are equal.
 */
static inline bool span_cmp(span_t a, span_t b)
{
	return a.start == b.start && a.end == b.end;
}

/**
 * @brief Merge two spans
 *
 * @example
 * /// for assert
 * #include <assert.h>
 *
 * auto a = span(0, 10);
 * auto b = span(10, 16);
 * auto span = span_merge(a, b);
 * assert(span_cmp(span, span(0, 16)));
 */
static inline span_t span_merge(span_t a, span_t b)
{
	usize start = (a.start < b.start) ? a.start : b.start;
	usize end = (a.end > b.end) ? a.end : b.end;
	return span(start, end);
}

/**
 * @brief Get the length of the span.
 */
static inline usize span_len(span_t span)
{
	return span.end - span.start;
}

/**
 * @brief Iter a span.
 *
 * @note This macro is type-safe and ensures the second argument
 * is exactly of type `span_t` at compile-time.
 *
 * @example
 * /// iter from 0 to 9
 * for_span_in(i, span(0, 10))
 * {
 * dbg("i = %zu", i);
 * }
 */
#define for_span(var, span)                                              \
	static_assert(is_type(span, span_t),                             \
		      "for_span_in: argument must be of type 'span_t'"); \
	for (usize var = (span).start; var < (span).end; ++var)

/**
 * @brief Iter from start to end.
 *
 * @note This macro is type-safe. It uses `typeof` to infer the iterator's
 * type from `start` and uses `static_assert` to ensure `start` and `end`
 * types are compatible.
 *
 * @example
 * /// iter from 5 to 9
 * foreach(i, 5, 10)
 * {
 *	dbg("i = %zu", i);
 * }
 *
 * /// iter from -5 to -1
 * foreach(i, -5, 0)
 * {
 *	dbg("i = %d", i);
 * }
 */
#define foreach(var, start, end)                                          \
	static_assert(                                                    \
		typecmp(start, end),                                      \
		"foreach: 'start' and 'end' must have compatible types"); \
	for (typeof(start) var = (start); var < (end); ++var)
