#ifndef TIDE_LIMITS_H
#define TIDE_LIMITS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIDE_MAX_DEFAULT_DEPTH 16u
#define TIDE_MAX_FULL_DEPTH 32u
#define TIDE_HEADER_SIZE 72u
#define TIDE_UUID_SIZE 16u
#define TIDE_DIGEST_SIZE 32u

typedef struct tide_limits {
  uint64_t max_file_size;
  uint64_t max_record_size;
  uint64_t max_payload_size;
  size_t max_single_alloc;
  size_t max_aggregate_alloc;
  uint32_t max_depth;
  uint32_t max_tracks;
  uint32_t max_edit_entries;
  uint32_t max_packet_table_entries;
  uint32_t reorder_depth;
} tide_limits;

tide_limits tide_limits_default(void);
tide_limits tide_limits_full(void);

#ifdef __cplusplus
}
#endif

#endif
