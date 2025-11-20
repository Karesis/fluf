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

/// for fmt, eq
#include <core/type.h>
#include <core/macros.h>

/// for stderr, fprintf
#include <stdio.h>
/// for clock_t, clock
#include <time.h>
#include <stdbool.h>
/// for fork, waitpid
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/*
 * ==========================================================================
 * Colors
 * ==========================================================================
 */

#define TEST_COLOR_RESET "\033[0m"
#define TEST_COLOR_RED "\033[31m"
#define TEST_COLOR_GREEN "\033[32m"
#define TEST_COLOR_YELLOW "\033[33m"
#define TEST_COLOR_BLUE "\033[34m"

/*
 * ==========================================================================
 * Global State (Per Test Suite)
 * ==========================================================================
 */

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/*
 * ==========================================================================
 * Assertions
 * ==========================================================================
 */

/**
 * @brief Basic test assertion.
 * If fails, prints error and returns from the current test function.
 */
#define expect(cond)                                                       \
	do {                                                               \
		if (!(cond)) {                                             \
			fprintf(stderr,                                    \
				"    %s[FAILED]%s Assertion failed: %s\n", \
				TEST_COLOR_RED, TEST_COLOR_RESET, #cond);  \
			fprintf(stderr, "           at %s:%d\n", __FILE__, \
				__LINE__);                                 \
			return false;                                      \
		}                                                          \
	} while (0)

/**
 * @brief Generic Equality Assertion.
 */
#define expect_eq(expected, actual)                                            \
	do {                                                                   \
		auto _lhs = (expected);                                        \
		auto _rhs = (actual);                                          \
                                                                               \
		if (unlikely(!eq(_lhs, _rhs))) {                               \
			fprintf(stderr, "    %s[FAILED]%s eq check failed:\n", \
				TEST_COLOR_RED, TEST_COLOR_RESET);             \
                                                                               \
			fprintf(stderr, "           Expected ");               \
			fprintf(stderr, fmt(_lhs), _lhs);                      \
			fprintf(stderr, ", got ");                             \
			fprintf(stderr, fmt(_rhs), _rhs);                      \
                                                                               \
			fprintf(stderr, "\n           at %s:%d\n", __FILE__,   \
				__LINE__);                                     \
			return false;                                          \
		}                                                              \
	} while (0)

/**
 * @brief Internal helper to analyze the child process exit status.
 *
 * @param status The status code returned by waitpid.
 * @param file   The source file name (for error reporting).
 * @param line   The line number (for error reporting).
 * @return true if the process aborted (SIGABRT), false otherwise.
 */
static inline bool _test_check_panic_status(int status, const char *file,
					    int line)
{
	// Case 1: The process was terminated by a signal (Crashed/Killed)
	if (WIFSIGNALED(status)) {
		int sig = WTERMSIG(status);

		if (sig == SIGABRT) {
			// Expected behavior: The assertion triggered abort().
			return true;
		}

		if (sig == SIGSEGV) {
			// Unexpected behavior: Segmentation Fault (Memory violation).
			fprintf(stderr,
				"    %s[FAILED]%s Crash detected, but it was SIGSEGV (SegFault)!\n",
				TEST_COLOR_RED, TEST_COLOR_RESET);
			fprintf(stderr,
				"            > This usually means invalid pointer access.\n");
			fprintf(stderr,
				"            > Check pointers at %s:%d\n", file,
				line);
			return false;
		}

		// Other signals (SIGILL, SIGFPE, etc.)
		fprintf(stderr,
			"    %s[FAILED]%s Process died with unexpected signal: %d (%s)\n",
			TEST_COLOR_RED, TEST_COLOR_RESET, sig, strsignal(sig));
		fprintf(stderr, "            at %s:%d\n", file, line);
		return false;
	}

	// Case 2: The process exited normally (did not panic)
	fprintf(stderr,
		"    %s[FAILED]%s Expected panic, but code exited normally.\n",
		TEST_COLOR_RED, TEST_COLOR_RESET);
	fprintf(stderr, "            at %s:%d\n", file, line);

	return false;
}

/**
 * @brief Death Test Assertion.
 * Expects the expression `stmt` to cause the program to panic (abort).
 *
 * Logic:
 * 1. Fork a child process.
 * 2. Child executes `stmt`.
 * 3. Parent waits and uses `_test_check_panic_status` to verify the outcome.
 */
#define expect_panic(stmt)                                                  \
	do {                                                                \
		pid_t pid = fork();                                         \
		if (pid == 0) {                                             \
			/* --- Child Process --- */                         \
			/* Silence stderr to keep test output clean */      \
			freopen("/dev/null", "w", stderr);                  \
			stmt;                                               \
			/* If we reach here, the panic failed to trigger */ \
			exit(0);                                            \
		} else if (pid > 0) {                                       \
			/* --- Parent Process --- */                        \
			int status;                                         \
			waitpid(pid, &status, 0);                           \
			/* Delegate logic to the helper function */         \
			if (!_test_check_panic_status(status, __FILE__,     \
						      __LINE__)) {          \
				return false;                               \
			}                                                   \
		} else {                                                    \
			perror("fork failed");                              \
			return false;                                       \
		}                                                           \
	} while (0)

/*
 * ==========================================================================
 * Framework Macros
 * ==========================================================================
 */

/**
 * @brief Define a test case.
 * @example
 * TEST(my_feature) 
 * {
 *     expect(1 + 1 == 2);
 *     return true; // Pass
 * }
 */
#define TEST(name) static bool test_##name(void)

/**
 * @brief Run a test case.
 * @example
 * int main() 
 * {
 *     RUN(my_feature);
 * }
 */
#define RUN(name)                                                          \
	do {                                                               \
		g_tests_run++;                                             \
		fprintf(stderr, "test %-30s ... ", #name);                 \
                                                                           \
		clock_t start = clock();                                   \
		bool passed = test_##name();                               \
		clock_t end = clock();                                     \
		double time_ms =                                           \
			((double)(end - start)) / CLOCKS_PER_SEC * 1000.0; \
                                                                           \
		if (passed) {                                              \
			g_tests_passed++;                                  \
			fprintf(stderr, "%sok%s", TEST_COLOR_GREEN,        \
				TEST_COLOR_RESET);                         \
		} else {                                                   \
			g_tests_failed++;                                  \
			fprintf(stderr, "%sFAILED%s", TEST_COLOR_RED,      \
				TEST_COLOR_RESET);                         \
		}                                                          \
		fprintf(stderr, " \t(%.2fms)\n", time_ms);                 \
	} while (0)

/**
 * @brief Print summary at the end of main.
 */
#define SUMMARY()                                                                \
	do {                                                                     \
		fprintf(stderr,                                                  \
			"\nTest result: %s. %d passed; %d failed; %d total\n\n", \
			g_tests_failed == 0 ?                                    \
				TEST_COLOR_GREEN "ok" TEST_COLOR_RESET :         \
				TEST_COLOR_RED "FAILED" TEST_COLOR_RESET,        \
			g_tests_passed, g_tests_failed, g_tests_run);            \
		return g_tests_failed != 0;                                      \
	} while (0)

/**
 * @brief Run code block with stderr silenced.
 * * Logic:
 * 1. Save the current stderr file descriptor (dup).
 * 2. Redirect stderr to /dev/null (freopen).
 * 3. Run the statement.
 * 4. Restore stderr from the saved descriptor (dup2).
 */
#define silence(stmt)                                            \
	do {                                                     \
		/* 1. Flush to ensure no pending data is lost */ \
		fflush(stderr);                                  \
		/* 2. Save original stderr fd */                 \
		int _saved_stderr = dup(STDERR_FILENO);          \
		/* 3. Redirect to /dev/null */                   \
		FILE *_null = freopen("/dev/null", "w", stderr); \
		unused(_null); /* suppress unused warning */     \
                                                                 \
		/* 4. Run the code */                            \
		stmt;                                            \
                                                                 \
		/* 5. Flush again */                             \
		fflush(stderr);                                  \
		/* 6. Restore stderr */                          \
		dup2(_saved_stderr, STDERR_FILENO);              \
		close(_saved_stderr);                            \
	} while (0)
