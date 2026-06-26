#include "test_harness.h"

#include "tide/tide.h"

static int edit_clip_produces_expected_piece(void) {
  tide_edit_entry edit = {100, 50, 10, {1u, 1u}};
  int64_t out_pts = 0;
  int64_t out_duration = 0;
  TIDE_EXPECT_STATUS(tide_edit_project_one(&edit, 1u, 0, 20, &out_pts, &out_duration), TIDE_STATUS_OK);
  TIDE_EXPECT(out_pts == 100);
  TIDE_EXPECT(out_duration == 10);
  return 0;
}

static int edit_gap_drops_packet(void) {
  tide_edit_entry edit = {0, 10, 100, {1u, 1u}};
  int64_t out_pts = 0;
  int64_t out_duration = 0;
  TIDE_EXPECT_STATUS(tide_edit_project_one(&edit, 1u, 0, 10, &out_pts, &out_duration), TIDE_STATUS_PARTIAL);
  return 0;
}

int tide_edit_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"EditClipProducesExpectedPieces", edit_clip_produces_expected_piece};
  if (max > n) out[n++] = (tide_test_case){"EditGapDropsPacket", edit_gap_drops_packet};
  return n;
}
