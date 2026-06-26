#ifndef TIDE_TEST_HARNESS_H
#define TIDE_TEST_HARNESS_H

#include <stdio.h>

typedef int (*tide_test_fn)(void);

typedef struct tide_test_case {
  const char *name;
  tide_test_fn fn;
} tide_test_case;

#define TIDE_EXPECT(expr) \
  do { \
    if (!(expr)) { \
      fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, #expr); \
      return 1; \
    } \
  } while (0)

#define TIDE_EXPECT_STATUS(actual, expected) \
  do { \
    tide_status status_value = (actual); \
    if (status_value != (expected)) { \
      fprintf(stderr, "%s:%d: expected %s got %s\n", __FILE__, __LINE__, \
              tide_status_string(expected), tide_status_string(status_value)); \
      return 1; \
    } \
  } while (0)

int tide_reader_tests(tide_test_case *out, int max);
int tide_record_semantics_tests(tide_test_case *out, int max);
int tide_time_tests(tide_test_case *out, int max);
int tide_edit_tests(tide_test_case *out, int max);
int tide_reorder_tests(tide_test_case *out, int max);
int tide_partial_file_tests(tide_test_case *out, int max);
int tide_remux_tests(tide_test_case *out, int max);
int tide_repair_tests(tide_test_case *out, int max);

#endif
