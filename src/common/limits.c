#include "tide/limits.h"

tide_limits tide_limits_default(void) {
  tide_limits limits;
  limits.max_file_size = 512ull * 1024ull * 1024ull;
  limits.max_record_size = 64ull * 1024ull * 1024ull;
  limits.max_payload_size = 64ull * 1024ull * 1024ull;
  limits.max_single_alloc = 32u * 1024u * 1024u;
  limits.max_aggregate_alloc = 128u * 1024u * 1024u;
  limits.max_depth = TIDE_MAX_DEFAULT_DEPTH;
  limits.max_tracks = 64u;
  limits.max_edit_entries = 256u;
  limits.max_packet_table_entries = 1000000u;
  limits.reorder_depth = 16u;
  return limits;
}

tide_limits tide_limits_full(void) {
  tide_limits limits = tide_limits_default();
  limits.max_file_size = 8ull * 1024ull * 1024ull * 1024ull;
  limits.max_payload_size = 1024ull * 1024ull * 1024ull;
  limits.max_aggregate_alloc = 512u * 1024u * 1024u;
  limits.max_depth = TIDE_MAX_FULL_DEPTH;
  limits.max_tracks = 1024u;
  limits.max_edit_entries = 1024u;
  limits.reorder_depth = 64u;
  return limits;
}
