#include "tide_internal.h"

#include <string.h>

static const uint8_t tide_magic[8] = {
  TIDE_MAGIC_0, TIDE_MAGIC_1, TIDE_MAGIC_2, TIDE_MAGIC_3,
  TIDE_MAGIC_4, TIDE_MAGIC_5, TIDE_MAGIC_6, TIDE_MAGIC_7
};

tide_status tide_write_header(tide_buffer *out,
                              tide_rational movie_time_base,
                              const uint8_t uuid[TIDE_UUID_SIZE]) {
  uint8_t header[TIDE_HEADER_SIZE];
  uint32_t crc;
  uint8_t zero_uuid[TIDE_UUID_SIZE] = {0};

  if (out == NULL || uuid == NULL || !tide_rational_is_valid(movie_time_base) ||
      memcmp(uuid, zero_uuid, sizeof(zero_uuid)) == 0) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  memset(header, 0, sizeof(header));
  memcpy(header, tide_magic, sizeof(tide_magic));
  tide_write_u16(header + 8, 1u);
  tide_write_u16(header + 10, 0u);
  memcpy(header + 12, uuid, TIDE_UUID_SIZE);
  tide_write_u32(header + 28, movie_time_base.numerator);
  tide_write_u32(header + 32, movie_time_base.denominator);
  tide_write_u64(header + 36, 0u);
  tide_write_u64(header + 44, 0u);
  tide_write_u64(header + 52, 64ull * 1024ull * 1024ull);
  tide_write_u64(header + 60, 0u);
  crc = tide_crc32c(header, TIDE_HEADER_SIZE - 4u);
  tide_write_u32(header + 68, crc);
  return tide_buffer_append(out, header, sizeof(header));
}

tide_status tide_write_record(tide_buffer *out,
                              uint16_t type,
                              uint16_t flags,
                              uint8_t alignment_log2,
                              uint64_t sequence,
                              const uint8_t *payload,
                              size_t payload_size) {
  uint8_t prefix[32];
  size_t payload_var_size;
  size_t seq_var_size;
  size_t header_size;
  uint64_t unpadded_size;
  uint64_t padded_size;
  size_t padding_size;
  uint32_t crc;
  size_t crc_start;
  tide_status status;

  if (out == NULL || (payload == NULL && payload_size != 0u) || alignment_log2 > 4u) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  payload_var_size = tide_uleb128_size((uint64_t)payload_size);
  seq_var_size = tide_uleb128_size(sequence);
  header_size = 8u + payload_var_size + seq_var_size;
  if (header_size > UINT16_MAX) {
    return TIDE_STATUS_RESOURCE;
  }

  tide_write_u16(prefix, type);
  tide_write_u16(prefix + 2, flags);
  tide_write_u16(prefix + 4, (uint16_t)header_size);
  prefix[6] = alignment_log2;
  prefix[7] = 0u;
  (void)tide_write_uleb128(prefix + 8, (uint64_t)payload_size);
  (void)tide_write_uleb128(prefix + 8u + payload_var_size, sequence);

  if (!tide_checked_add_u64((uint64_t)header_size, (uint64_t)payload_size, &unpadded_size) ||
      !tide_align_u64(unpadded_size, 1ull << alignment_log2, &padded_size)) {
    return TIDE_STATUS_RESOURCE;
  }
  padding_size = (size_t)(padded_size - unpadded_size);

  crc_start = out->size;
  status = tide_buffer_append(out, prefix, header_size);
  if (status == TIDE_STATUS_OK) {
    status = tide_buffer_append(out, payload, payload_size);
  }
  if (status == TIDE_STATUS_OK) {
    status = tide_buffer_append_zero(out, padding_size);
  }
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  crc = tide_crc32c(out->data + crc_start, out->size - crc_start);
  prefix[0] = (uint8_t)(crc & 0xffu);
  prefix[1] = (uint8_t)((crc >> 8u) & 0xffu);
  prefix[2] = (uint8_t)((crc >> 16u) & 0xffu);
  prefix[3] = (uint8_t)((crc >> 24u) & 0xffu);
  return tide_buffer_append(out, prefix, 4u);
}

tide_status tide_encode_stream_payload(tide_buffer *out,
                                       const tide_stream_info *stream) {
  uint8_t fixed[24];
  uint8_t varint[10];
  size_t varint_size;
  tide_status status;
  if (out == NULL || stream == NULL || !tide_rational_is_valid(stream->time_base) ||
      (stream->config == NULL && stream->config_size != 0u)) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  tide_write_u32(fixed, stream->track_id);
  tide_write_u32(fixed + 4, stream->generation);
  tide_write_u16(fixed + 8, stream->media_kind);
  tide_write_u16(fixed + 10, 0u);
  tide_write_u32(fixed + 12, stream->codec_tag);
  tide_write_u32(fixed + 16, stream->time_base.numerator);
  tide_write_u32(fixed + 20, stream->time_base.denominator);
  status = tide_buffer_append(out, fixed, sizeof(fixed));
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  varint_size = tide_write_uleb128(varint, (uint64_t)stream->config_size);
  status = tide_buffer_append(out, varint, varint_size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  return tide_buffer_append(out, stream->config, stream->config_size);
}

tide_status tide_encode_packet_payload(tide_buffer *out,
                                       const tide_packet_info *packet) {
  uint8_t fixed[TIDE_PACKET_FIXED_SIZE];
  uint8_t varint[10];
  size_t varint_size;
  tide_status status;

  if (out == NULL || packet == NULL ||
      (packet->payload.data == NULL && packet->payload.size != 0u) ||
      packet->duration < 0) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  tide_write_u32(fixed, packet->track_id);
  tide_write_u32(fixed + 4, packet->generation);
  tide_write_u64(fixed + 8, packet->packet_seq);
  tide_write_i64(fixed + 16, packet->dts);
  tide_write_i64(fixed + 24, packet->pts);
  tide_write_i64(fixed + 32, packet->duration);
  tide_write_u32(fixed + 40, packet->flags);
  status = tide_buffer_append(out, fixed, sizeof(fixed));
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  varint_size = tide_write_uleb128(varint, (uint64_t)packet->payload.size);
  status = tide_buffer_append(out, varint, varint_size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  status = tide_buffer_append(out, packet->payload.data, packet->payload.size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  return TIDE_STATUS_OK;
}
