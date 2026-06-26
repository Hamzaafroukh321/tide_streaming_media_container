#include "test_harness.h"

#include <stdio.h>
#include <string.h>

static int append_tests(tide_test_case *tests,
                        int count,
                        int max,
                        int (*loader)(tide_test_case *, int)) {
  int added;
  if (count >= max) {
    return count;
  }
  added = loader(tests + count, max - count);
  return count + added;
}

int main(void) {
  tide_test_case tests[128];
  int count = 0;
  int failed = 0;
  int i;
  memset(tests, 0, sizeof(tests));
  count = append_tests(tests, count, 128, tide_reader_tests);
  count = append_tests(tests, count, 128, tide_time_tests);
  count = append_tests(tests, count, 128, tide_edit_tests);
  count = append_tests(tests, count, 128, tide_reorder_tests);
  count = append_tests(tests, count, 128, tide_partial_file_tests);
  count = append_tests(tests, count, 128, tide_remux_tests);
  count = append_tests(tests, count, 128, tide_repair_tests);
  for (i = 0; i < count; ++i) {
    int result = tests[i].fn();
    if (result != 0) {
      fprintf(stderr, "FAIL %s\n", tests[i].name);
      failed += 1;
    } else {
      printf("PASS %s\n", tests[i].name);
    }
  }
  return failed == 0 ? 0 : 1;
}
