#include "tide/tide.h"

#include <limits.h>

tide_status tide_edit_project_one(const tide_edit_entry *edits,
                                  size_t edit_count,
                                  int64_t packet_pts,
                                  int64_t packet_duration,
                                  int64_t *out_pts,
                                  int64_t *out_duration) {
  size_t i;
  if (edits == NULL || out_pts == NULL || out_duration == NULL || packet_duration < 0) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  for (i = 0; i < edit_count; ++i) {
    int64_t edit_src_end;
    int64_t pkt_end;
    int64_t start;
    int64_t end;
    if (!tide_rational_is_valid(edits[i].rate) ||
        edits[i].output_duration < 0 ||
        edits[i].source_start > INT64_MAX - edits[i].output_duration ||
        packet_pts > INT64_MAX - packet_duration) {
      return TIDE_STATUS_TIMELINE;
    }
    edit_src_end = edits[i].source_start + edits[i].output_duration;
    pkt_end = packet_pts + packet_duration;
    start = packet_pts > edits[i].source_start ? packet_pts : edits[i].source_start;
    end = pkt_end < edit_src_end ? pkt_end : edit_src_end;
    if (start < end) {
      if (edits[i].output_start > INT64_MAX - (start - edits[i].source_start)) {
        return TIDE_STATUS_TIMELINE;
      }
      *out_pts = edits[i].output_start + (start - edits[i].source_start);
      *out_duration = end - start;
      return TIDE_STATUS_OK;
    }
  }
  return TIDE_STATUS_PARTIAL;
}
