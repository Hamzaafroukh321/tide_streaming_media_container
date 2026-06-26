#include "test_harness.h"

#include "tide/decoder.h"
#include "tide_internal.h"

#include <string.h>

static tide_status append_test_header_and_stream(tide_buffer *file) {
  uint8_t uuid[TIDE_UUID_SIZE] = {
    0x54u, 0x49u, 0x44u, 0x45u, 0x2du, 0x72u, 0x65u, 0x63u,
    0x6fu, 0x72u, 0x64u, 0x2du, 0x30u, 0x31u, 0x00u, 0x01u
  };
  tide_buffer payload = {0};
  tide_stream_info stream;
  tide_status status;
  file->aggregate_limit = 1024u * 1024u;
  payload.aggregate_limit = 1024u;
  status = tide_write_header(file, (tide_rational){1u, 1000u}, uuid);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  memset(&stream, 0, sizeof(stream));
  stream.track_id = 3u;
  stream.generation = 1u;
  stream.media_kind = 1u;
  stream.codec_tag = 0x52454344u;
  stream.time_base = (tide_rational){1u, 48000u};
  status = tide_encode_stream_payload(&payload, &stream);
  if (status == TIDE_STATUS_OK) {
    status = tide_write_record(file, TIDE_RECORD_STREAM_DESC, 0u, 3u, 0u, payload.data, payload.size);
  }
  tide_buffer_destroy(&payload);
  return status;
}

static tide_status decode_buffer(const tide_buffer *file) {
  tide_error error;
  uint64_t valid_prefix = 0u;
  return tide_parse_tide_bytes(file->data, file->size, 1, NULL, NULL, NULL, &error, &valid_prefix);
}

static int edit_list_semantics_accept_ordered_entries(void) {
  tide_buffer file = {0};
  tide_buffer payload = {0};
  uint8_t count[10];
  size_t count_size;
  uint8_t entry[40];
  TIDE_EXPECT_STATUS(append_test_header_and_stream(&file), TIDE_STATUS_OK);
  payload.aggregate_limit = 1024u;
  TIDE_EXPECT_STATUS(tide_buffer_append_zero(&payload, 8u), TIDE_STATUS_OK);
  tide_write_u32(payload.data, 3u);
  tide_write_u32(payload.data + 4u, 1u);
  count_size = tide_write_uleb128(count, 1u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, count, count_size), TIDE_STATUS_OK);
  tide_write_i64(entry, 0);
  tide_write_i64(entry + 8u, 100);
  tide_write_i64(entry + 16u, 50);
  tide_write_u32(entry + 24u, 1u);
  tide_write_u32(entry + 28u, 1u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, entry, 32u), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_write_record(&file, TIDE_RECORD_EDIT_LIST, 0u, 3u, 1u, payload.data, payload.size), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(decode_buffer(&file), TIDE_STATUS_OK);
  tide_buffer_destroy(&payload);
  tide_buffer_destroy(&file);
  return 0;
}

static int edit_list_rejects_overlap(void) {
  tide_buffer file = {0};
  tide_buffer payload = {0};
  uint8_t count[10];
  size_t count_size;
  uint8_t entry[32];
  TIDE_EXPECT_STATUS(append_test_header_and_stream(&file), TIDE_STATUS_OK);
  payload.aggregate_limit = 1024u;
  TIDE_EXPECT_STATUS(tide_buffer_append_zero(&payload, 8u), TIDE_STATUS_OK);
  tide_write_u32(payload.data, 3u);
  tide_write_u32(payload.data + 4u, 1u);
  count_size = tide_write_uleb128(count, 2u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, count, count_size), TIDE_STATUS_OK);
  tide_write_i64(entry, 0);
  tide_write_i64(entry + 8u, 100);
  tide_write_i64(entry + 16u, 0);
  tide_write_u32(entry + 24u, 1u);
  tide_write_u32(entry + 28u, 1u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, entry, sizeof(entry)), TIDE_STATUS_OK);
  tide_write_i64(entry, 50);
  tide_write_i64(entry + 8u, 25);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, entry, sizeof(entry)), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_write_record(&file, TIDE_RECORD_EDIT_LIST, 0u, 3u, 1u, payload.data, payload.size), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(decode_buffer(&file), TIDE_STATUS_TIMELINE);
  tide_buffer_destroy(&payload);
  tide_buffer_destroy(&file);
  return 0;
}

static int packet_table_rejects_overlapping_ranges(void) {
  tide_buffer file = {0};
  tide_buffer payload = {0};
  uint8_t count[10];
  size_t count_size;
  uint8_t entry[56];
  TIDE_EXPECT_STATUS(append_test_header_and_stream(&file), TIDE_STATUS_OK);
  payload.aggregate_limit = 1024u;
  TIDE_EXPECT_STATUS(tide_buffer_append_zero(&payload, 8u), TIDE_STATUS_OK);
  tide_write_u32(payload.data, 3u);
  tide_write_u32(payload.data + 4u, 1u);
  count_size = tide_write_uleb128(count, 2u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, count, count_size), TIDE_STATUS_OK);
  memset(entry, 0, sizeof(entry));
  tide_write_u64(entry, 1u);
  tide_write_i64(entry + 24u, 10);
  tide_write_u64(entry + 36u, 0u);
  tide_write_u64(entry + 44u, 10u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, entry, 52u), TIDE_STATUS_OK);
  tide_write_u64(entry, 2u);
  tide_write_u64(entry + 36u, 5u);
  tide_write_u64(entry + 44u, 10u);
  TIDE_EXPECT_STATUS(tide_buffer_append(&payload, entry, 52u), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_write_record(&file, TIDE_RECORD_PACKET_TABLE, 0u, 3u, 1u, payload.data, payload.size), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(decode_buffer(&file), TIDE_STATUS_FORMAT);
  tide_buffer_destroy(&payload);
  tide_buffer_destroy(&file);
  return 0;
}

static int footer_rejects_wrong_file_length(void) {
  tide_buffer file = {0};
  uint8_t payload[49];
  TIDE_EXPECT_STATUS(append_test_header_and_stream(&file), TIDE_STATUS_OK);
  memset(payload, 0, sizeof(payload));
  tide_write_u64(payload, 123u);
  TIDE_EXPECT_STATUS(tide_write_record(&file, TIDE_RECORD_FOOTER, 0u, 3u, 1u, payload, sizeof(payload)), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(decode_buffer(&file), TIDE_STATUS_FORMAT);
  tide_buffer_destroy(&file);
  return 0;
}

int tide_record_semantics_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"EditListSemanticsAcceptOrderedEntries", edit_list_semantics_accept_ordered_entries};
  if (max > n) out[n++] = (tide_test_case){"EditListRejectsOverlap", edit_list_rejects_overlap};
  if (max > n) out[n++] = (tide_test_case){"PacketTableRejectsOverlappingRanges", packet_table_rejects_overlapping_ranges};
  if (max > n) out[n++] = (tide_test_case){"FooterRejectsWrongFileLength", footer_rejects_wrong_file_length};
  return n;
}
