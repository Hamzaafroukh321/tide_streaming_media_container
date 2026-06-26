#ifndef TIDE_TIDE_H
#define TIDE_TIDE_H

#include <stddef.h>
#include <stdint.h>

#include "tide/error.h"
#include "tide/limits.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tide_decoder {
  void *impl;
} tide_decoder;

typedef struct tide_source tide_source;
typedef struct tide_demux tide_demux;
typedef struct tide_mux tide_mux;
typedef struct tide_repair_plan tide_repair_plan;
typedef struct tide_payload_lease tide_payload_lease;

typedef enum tide_record_type {
  TIDE_RECORD_GROUP = 0x0001u,
  TIDE_RECORD_STREAM_DESC = 0x0010u,
  TIDE_RECORD_PACKET = 0x0020u,
  TIDE_RECORD_PACKET_TABLE = 0x0021u,
  TIDE_RECORD_EDIT_LIST = 0x0030u,
  TIDE_RECORD_DISCONTINUITY = 0x0031u,
  TIDE_RECORD_SEEK_INDEX = 0x0040u,
  TIDE_RECORD_CHECKPOINT = 0x0050u,
  TIDE_RECORD_INDEX_DIRECTORY = 0x0060u,
  TIDE_RECORD_FOOTER = 0x007Fu
} tide_record_type;

typedef enum tide_lifecycle_state {
  TIDE_LIFECYCLE_INIT = 0,
  TIDE_LIFECYCLE_ACTIVE = 1,
  TIDE_LIFECYCLE_SEALED = 2,
  TIDE_LIFECYCLE_FAILED = 3,
  TIDE_LIFECYCLE_DESTROYED = 4
} tide_lifecycle_state;

typedef enum tide_rounding {
  TIDE_ROUND_FLOOR = 0,
  TIDE_ROUND_CEIL = 1,
  TIDE_ROUND_NEAREST_EVEN = 2
} tide_rounding;

typedef struct tide_rational {
  uint32_t numerator;
  uint32_t denominator;
} tide_rational;

typedef struct tide_record_event {
  uint16_t type;
  uint16_t flags;
  uint64_t offset;
  uint64_t payload_offset;
  uint64_t payload_size;
  uint64_t sequence;
  uint8_t depth;
  int skippable;
} tide_record_event;

typedef struct tide_stream_info {
  uint32_t track_id;
  uint32_t generation;
  uint16_t media_kind;
  uint32_t codec_tag;
  tide_rational time_base;
  const uint8_t *config;
  size_t config_size;
} tide_stream_info;

typedef struct tide_payload_view {
  const uint8_t *data;
  size_t size;
  tide_payload_lease *lease;
} tide_payload_view;

typedef struct tide_packet_info {
  uint32_t track_id;
  uint32_t generation;
  uint64_t packet_seq;
  int64_t dts;
  int64_t pts;
  int64_t duration;
  uint32_t flags;
  tide_payload_view payload;
} tide_packet_info;

typedef struct tide_packet_ref {
  tide_packet_info info;
  tide_payload_lease *lease;
} tide_packet_ref;

typedef struct tide_edit_entry {
  int64_t output_start;
  int64_t output_duration;
  int64_t source_start;
  tide_rational rate;
} tide_edit_entry;

typedef struct tide_callbacks {
  size_t size;
  tide_status (*on_record)(void *user, const tide_record_event *event);
  tide_status (*on_stream)(void *user, const tide_stream_info *stream);
  tide_status (*on_packet)(void *user, const tide_packet_info *packet);
  void (*on_error)(void *user, const tide_error *error);
} tide_callbacks;

uint32_t tide_crc32c(const uint8_t *data, size_t size);

int tide_rational_is_valid(tide_rational value);
tide_status tide_rational_reduce(tide_rational *value);
tide_status tide_time_convert(int64_t value,
                              tide_rational source,
                              tide_rational destination,
                              tide_rounding rounding,
                              int64_t *out);
tide_status tide_edit_project_one(const tide_edit_entry *edits,
                                  size_t edit_count,
                                  int64_t packet_pts,
                                  int64_t packet_duration,
                                  int64_t *out_pts,
                                  int64_t *out_duration);

void tide_packet_ref_init(tide_packet_ref *packet);
void tide_packet_ref_reset(tide_packet_ref *packet);
tide_status tide_packet_ref_move(tide_packet_ref *dst, tide_packet_ref *src);

#ifdef __cplusplus
}
#endif

#endif
