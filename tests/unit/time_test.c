#include "test_harness.h"

#include "tide/tide.h"

static int rational_exact_conversion(void) {
  int64_t out = 0;
  TIDE_EXPECT_STATUS(tide_time_convert(48000, (tide_rational){1u, 48000u}, (tide_rational){1u, 1000u}, TIDE_ROUND_FLOOR, &out), TIDE_STATUS_OK);
  TIDE_EXPECT(out == 1000);
  return 0;
}

static int nearest_even_tie_cases(void) {
  int64_t out = 0;
  TIDE_EXPECT_STATUS(tide_time_convert(1, (tide_rational){1u, 2u}, (tide_rational){1u, 1u}, TIDE_ROUND_NEAREST_EVEN, &out), TIDE_STATUS_OK);
  TIDE_EXPECT(out == 0);
  TIDE_EXPECT_STATUS(tide_time_convert(3, (tide_rational){1u, 2u}, (tide_rational){1u, 1u}, TIDE_ROUND_NEAREST_EVEN, &out), TIDE_STATUS_OK);
  TIDE_EXPECT(out == 2);
  return 0;
}

static int invalid_time_base_rejected(void) {
  int64_t out = 0;
  TIDE_EXPECT_STATUS(tide_time_convert(1, (tide_rational){2u, 4u}, (tide_rational){1u, 1u}, TIDE_ROUND_FLOOR, &out), TIDE_STATUS_TIMELINE);
  return 0;
}

int tide_time_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"RationalExactConversion", rational_exact_conversion};
  if (max > n) out[n++] = (tide_test_case){"NearestEvenTieCases", nearest_even_tie_cases};
  if (max > n) out[n++] = (tide_test_case){"TimestampInvalidBaseRejected", invalid_time_base_rejected};
  return n;
}
