#include "tide_internal.h"

#include <limits.h>

int tide_rational_is_valid(tide_rational value) {
  return value.numerator != 0u &&
         value.denominator != 0u &&
         tide_gcd_u64(value.numerator, value.denominator) == 1u;
}

tide_status tide_rational_reduce(tide_rational *value) {
  uint64_t gcd;
  if (value == NULL || value->numerator == 0u || value->denominator == 0u) {
    return TIDE_STATUS_TIMELINE;
  }
  gcd = tide_gcd_u64(value->numerator, value->denominator);
  value->numerator = (uint32_t)((uint64_t)value->numerator / gcd);
  value->denominator = (uint32_t)((uint64_t)value->denominator / gcd);
  return TIDE_STATUS_OK;
}

static tide_status tide_round_div_i128(__int128 numerator,
                                       __int128 denominator,
                                       tide_rounding rounding,
                                       int64_t *out) {
  __int128 q;
  __int128 r;
  if (denominator <= 0 || out == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  q = numerator / denominator;
  r = numerator % denominator;
  if (r < 0) {
    r = -r;
  }
  if (rounding == TIDE_ROUND_FLOOR && numerator < 0 && r != 0) {
    --q;
  } else if (rounding == TIDE_ROUND_CEIL && numerator > 0 && r != 0) {
    ++q;
  } else if (rounding == TIDE_ROUND_NEAREST_EVEN) {
    __int128 twice = r * 2;
    if (twice > denominator || (twice == denominator && (q & 1) != 0)) {
      q += numerator >= 0 ? 1 : -1;
    }
  }
  if (q < INT64_MIN || q > INT64_MAX) {
    return TIDE_STATUS_TIMELINE;
  }
  *out = (int64_t)q;
  return TIDE_STATUS_OK;
}

tide_status tide_time_convert(int64_t value,
                              tide_rational source,
                              tide_rational destination,
                              tide_rounding rounding,
                              int64_t *out) {
  uint64_t gcd1;
  uint64_t gcd2;
  uint64_t num;
  uint64_t den;
  __int128 numerator;
  __int128 denominator;

  if (!tide_rational_is_valid(source) ||
      !tide_rational_is_valid(destination) ||
      out == NULL) {
    return TIDE_STATUS_TIMELINE;
  }
  num = source.numerator;
  den = source.denominator;
  gcd1 = tide_gcd_u64(num, destination.numerator);
  num /= gcd1;
  denominator = (__int128)(destination.numerator / gcd1);
  gcd2 = tide_gcd_u64(destination.denominator, den);
  denominator *= (__int128)(den / gcd2);
  numerator = (__int128)value * (__int128)num *
              (__int128)(destination.denominator / gcd2);
  return tide_round_div_i128(numerator, denominator, rounding, out);
}
