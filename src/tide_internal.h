#ifndef TIDE_INTERNAL_H
#define TIDE_INTERNAL_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "tide/demux.h"
#include "tide/mux.h"
#include "tide/repair.h"

#define TIDE_MAGIC_0 0x54u
#define TIDE_MAGIC_1 0x49u
#define TIDE_MAGIC_2 0x44u
#define TIDE_MAGIC_3 0x45u
#define TIDE_MAGIC_4 0x31u
#define TIDE_MAGIC_5 0x0Du
#define TIDE_MAGIC_6 0x0Au
#define TIDE_MAGIC_7 0x1Au
#define TIDE_RECORD_SKIPPABLE_BIT 0x8000u
#define TIDE_MAX_HEADER_VARINT_BYTES 10u
#define TIDE_GROUP_META_SIZE 20u
#define TIDE_PACKET_FIXED_SIZE 44u

typedef struct tide_reader {
  const uint8_t *data;
  size_t size;
  size_t offset;
} tide_reader;

typedef struct tide_buffer {
  uint8_t *data;
  size_t size;
  size_t capacity;
  size_t aggregate_limit;
} tide_buffer;

struct tide_source {
  uint8_t *data;
  size_t size;
  uint64_t generation;
};

typedef struct tide_track_snapshot {
  uint32_t track_id;
  uint32_t generation;
  uint16_t media_kind;
  uint32_t codec_tag;
  tide_rational time_base;
} tide_track_snapshot;

typedef struct tide_index_entry {
  uint32_t track_id;
  uint64_t packet_seq;
  int64_t pts;
  uint64_t record_offset;
} tide_index_entry;

typedef struct tide_reorder_item {
  tide_packet_ref packet;
  uint64_t epoch;
} tide_reorder_item;

typedef struct tide_reorder_queue {
  tide_reorder_item *items;
  size_t count;
  size_t capacity;
  size_t limit;
} tide_reorder_queue;

int tide_checked_add_u64(uint64_t a, uint64_t b, uint64_t *out);
int tide_checked_mul_u64(uint64_t a, uint64_t b, uint64_t *out);
int tide_checked_u64_to_size(uint64_t value, size_t *out);
int tide_align_u64(uint64_t value, uint64_t alignment, uint64_t *out);
uint64_t tide_gcd_u64(uint64_t a, uint64_t b);

void tide_reader_init(tide_reader *reader, const uint8_t *data, size_t size);
int tide_reader_read_u8(tide_reader *reader, uint8_t *out);
int tide_reader_read_u16(tide_reader *reader, uint16_t *out);
int tide_reader_read_u32(tide_reader *reader, uint32_t *out);
int tide_reader_read_u64(tide_reader *reader, uint64_t *out);
int tide_reader_read_i64(tide_reader *reader, int64_t *out);
int tide_reader_read_bytes(tide_reader *reader, const uint8_t **out, size_t size);
int tide_reader_read_uleb128(tide_reader *reader, uint64_t *out);
int tide_reader_skip(tide_reader *reader, size_t size);

void tide_write_u16(uint8_t *dst, uint16_t value);
void tide_write_u32(uint8_t *dst, uint32_t value);
void tide_write_u64(uint8_t *dst, uint64_t value);
void tide_write_i64(uint8_t *dst, int64_t value);
size_t tide_write_uleb128(uint8_t *dst, uint64_t value);
size_t tide_uleb128_size(uint64_t value);

tide_status tide_buffer_reserve(tide_buffer *buffer, size_t additional);
tide_status tide_buffer_append(tide_buffer *buffer, const void *data, size_t size);
tide_status tide_buffer_append_zero(tide_buffer *buffer, size_t size);
void tide_buffer_destroy(tide_buffer *buffer);

tide_status tide_write_header(tide_buffer *out,
                              tide_rational movie_time_base,
                              const uint8_t uuid[TIDE_UUID_SIZE]);
tide_status tide_write_record(tide_buffer *out,
                              uint16_t type,
                              uint16_t flags,
                              uint8_t alignment_log2,
                              uint64_t sequence,
                              const uint8_t *payload,
                              size_t payload_size);
tide_status tide_encode_stream_payload(tide_buffer *out,
                                       const tide_stream_info *stream);
tide_status tide_encode_packet_payload(tide_buffer *out,
                                       const tide_packet_info *packet);

tide_status tide_parse_tide_bytes(const uint8_t *data,
                                  size_t size,
                                  int end,
                                  const tide_limits *limits,
                                  const tide_callbacks *callbacks,
                                  void *user,
                                  tide_error *error,
                                  uint64_t *valid_prefix);

tide_status tide_reorder_queue_init(tide_reorder_queue *queue, size_t limit);
void tide_reorder_queue_destroy(tide_reorder_queue *queue);
tide_status tide_reorder_queue_push(tide_reorder_queue *queue,
                                    tide_packet_ref *packet,
                                    uint64_t epoch);
tide_status tide_reorder_queue_pop_ready(tide_reorder_queue *queue,
                                         tide_packet_ref *out);

tide_status tide_index_builder_add(tide_index_entry **entries,
                                   size_t *count,
                                   size_t *capacity,
                                   const tide_index_entry *entry);

#endif
