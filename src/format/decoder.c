#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct tide_decoder_impl {
  tide_limits limits;
  tide_callbacks callbacks;
  void *user;
  tide_buffer buffer;
  uint64_t valid_prefix;
  int cancelled;
  int terminal;
  tide_error error;
  tide_track_snapshot *tracks;
  size_t track_count;
  size_t track_capacity;
} tide_decoder_impl;

typedef struct tide_record_frame {
  uint16_t type;
  uint16_t flags;
  uint16_t header_size;
  uint8_t alignment_log2;
  uint64_t payload_size;
  uint64_t sequence;
  size_t offset;
  size_t payload_offset;
  size_t payload_end;
  size_t padding_end;
  size_t crc_end;
  uint8_t depth;
} tide_record_frame;

static const uint8_t tide_magic_bytes[8] = {
  TIDE_MAGIC_0, TIDE_MAGIC_1, TIDE_MAGIC_2, TIDE_MAGIC_3,
  TIDE_MAGIC_4, TIDE_MAGIC_5, TIDE_MAGIC_6, TIDE_MAGIC_7
};

static tide_decoder_impl *tide_impl(tide_decoder *decoder) {
  return decoder == NULL ? NULL : (tide_decoder_impl *)decoder->impl;
}

static void tide_set_parse_error(tide_error *error,
                                 tide_status status,
                                 uint64_t offset,
                                 uint16_t type,
                                 uint8_t depth,
                                 const char *message) {
  tide_error_set(error, status, TIDE_SEVERITY_ERROR, offset, type, depth, "decoder", message);
}

static tide_status tide_validate_header(const uint8_t *data,
                                        size_t size,
                                        const tide_limits *limits,
                                        tide_error *error) {
  tide_reader reader;
  uint16_t major;
  uint16_t minor;
  const uint8_t *uuid;
  tide_rational movie_base;
  uint64_t required_features;
  uint64_t optional_features;
  uint64_t max_record_hint;
  uint64_t first_checkpoint_offset;
  uint32_t stored_crc;
  uint32_t actual_crc;
  uint8_t zero_uuid[TIDE_UUID_SIZE] = {0};
  (void)minor;
  (void)optional_features;
  (void)first_checkpoint_offset;

  if (size < TIDE_HEADER_SIZE) {
    tide_set_parse_error(error, TIDE_STATUS_TRUNCATED, size, 0, 0, "incomplete header");
    return TIDE_STATUS_TRUNCATED;
  }
  if (memcmp(data, tide_magic_bytes, sizeof(tide_magic_bytes)) != 0) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, 0, 0, 0, "bad magic");
    return TIDE_STATUS_FORMAT;
  }
  tide_reader_init(&reader, data + 8u, TIDE_HEADER_SIZE - 8u);
  if (!tide_reader_read_u16(&reader, &major) ||
      !tide_reader_read_u16(&reader, &minor) ||
      !tide_reader_read_bytes(&reader, &uuid, TIDE_UUID_SIZE) ||
      !tide_reader_read_u32(&reader, &movie_base.numerator) ||
      !tide_reader_read_u32(&reader, &movie_base.denominator) ||
      !tide_reader_read_u64(&reader, &required_features) ||
      !tide_reader_read_u64(&reader, &optional_features) ||
      !tide_reader_read_u64(&reader, &max_record_hint) ||
      !tide_reader_read_u64(&reader, &first_checkpoint_offset) ||
      !tide_reader_read_u32(&reader, &stored_crc)) {
    tide_set_parse_error(error, TIDE_STATUS_INTERNAL, 0, 0, 0, "header reader failed");
    return TIDE_STATUS_INTERNAL;
  }
  if (major != 1u) {
    tide_set_parse_error(error, TIDE_STATUS_UNSUPPORTED, 8, 0, 0, "unsupported major version");
    return TIDE_STATUS_UNSUPPORTED;
  }
  if (memcmp(uuid, zero_uuid, sizeof(zero_uuid)) == 0) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, 12, 0, 0, "zero file uuid");
    return TIDE_STATUS_FORMAT;
  }
  if (!tide_rational_is_valid(movie_base)) {
    tide_set_parse_error(error, TIDE_STATUS_TIMELINE, 28, 0, 0, "invalid movie time base");
    return TIDE_STATUS_TIMELINE;
  }
  if (required_features != 0u) {
    tide_set_parse_error(error, TIDE_STATUS_UNSUPPORTED, 36, 0, 0, "unknown required feature");
    return TIDE_STATUS_UNSUPPORTED;
  }
  if (max_record_hint > limits->max_record_size) {
    tide_set_parse_error(error, TIDE_STATUS_RESOURCE, 52, 0, 0, "record hint exceeds policy");
    return TIDE_STATUS_RESOURCE;
  }
  actual_crc = tide_crc32c(data, TIDE_HEADER_SIZE - 4u);
  if (stored_crc != actual_crc) {
    tide_set_parse_error(error, TIDE_STATUS_INTEGRITY, 68, 0, 0, "header crc mismatch");
    return TIDE_STATUS_INTEGRITY;
  }
  return TIDE_STATUS_OK;
}

static tide_track_snapshot *tide_find_track(tide_decoder_impl *impl,
                                            uint32_t track_id,
                                            uint32_t generation) {
  size_t i;
  for (i = 0; i < impl->track_count; ++i) {
    if (impl->tracks[i].track_id == track_id && impl->tracks[i].generation == generation) {
      return &impl->tracks[i];
    }
  }
  return NULL;
}

static tide_status tide_add_track(tide_decoder_impl *impl,
                                  const tide_stream_info *stream,
                                  tide_error *error,
                                  uint64_t offset) {
  tide_track_snapshot *grown;
  if (tide_find_track(impl, stream->track_id, stream->generation) != NULL) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, offset, TIDE_RECORD_STREAM_DESC, 0, "duplicate stream generation");
    return TIDE_STATUS_FORMAT;
  }
  if (impl->track_count >= impl->limits.max_tracks) {
    tide_set_parse_error(error, TIDE_STATUS_RESOURCE, offset, TIDE_RECORD_STREAM_DESC, 0, "track limit exceeded");
    return TIDE_STATUS_RESOURCE;
  }
  if (impl->track_count == impl->track_capacity) {
    size_t next = impl->track_capacity == 0u ? 8u : impl->track_capacity * 2u;
    grown = (tide_track_snapshot *)realloc(impl->tracks, next * sizeof(*grown));
    if (grown == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    impl->tracks = grown;
    impl->track_capacity = next;
  }
  impl->tracks[impl->track_count].track_id = stream->track_id;
  impl->tracks[impl->track_count].generation = stream->generation;
  impl->tracks[impl->track_count].media_kind = stream->media_kind;
  impl->tracks[impl->track_count].codec_tag = stream->codec_tag;
  impl->tracks[impl->track_count].time_base = stream->time_base;
  impl->track_count += 1u;
  return TIDE_STATUS_OK;
}

static tide_status tide_decode_record_prefix(const uint8_t *data,
                                             size_t size,
                                             size_t offset,
                                             uint8_t depth,
                                             tide_record_frame *frame,
                                             tide_error *error) {
  tide_reader reader;
  uint64_t record_payload_end_u64;
  uint64_t payload_end_u64;
  uint64_t record_padded_end_u64;
  uint64_t padding_end_u64;
  uint64_t crc_end_u64;
  if (size - offset < 8u) {
    return TIDE_STATUS_TRUNCATED;
  }
  tide_reader_init(&reader, data + offset, size - offset);
  if (!tide_reader_read_u16(&reader, &frame->type) ||
      !tide_reader_read_u16(&reader, &frame->flags) ||
      !tide_reader_read_u16(&reader, &frame->header_size) ||
      !tide_reader_read_u8(&reader, &frame->alignment_log2)) {
    return TIDE_STATUS_TRUNCATED;
  }
  if (!tide_reader_skip(&reader, 1u) ||
      !tide_reader_read_uleb128(&reader, &frame->payload_size) ||
      !tide_reader_read_uleb128(&reader, &frame->sequence)) {
    return TIDE_STATUS_TRUNCATED;
  }
  if (frame->header_size != reader.offset || frame->alignment_log2 > 4u) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)offset, frame->type, depth, "invalid record prefix");
    return TIDE_STATUS_FORMAT;
  }
  frame->offset = offset;
  frame->payload_offset = offset + frame->header_size;
  if (!tide_checked_add_u64((uint64_t)frame->header_size, frame->payload_size, &record_payload_end_u64) ||
      !tide_checked_add_u64((uint64_t)frame->offset, record_payload_end_u64, &payload_end_u64) ||
      !tide_align_u64(record_payload_end_u64, 1ull << frame->alignment_log2, &record_padded_end_u64) ||
      !tide_checked_add_u64((uint64_t)frame->offset, record_padded_end_u64, &padding_end_u64) ||
      !tide_checked_add_u64(padding_end_u64, 4u, &crc_end_u64) ||
      payload_end_u64 > (uint64_t)SIZE_MAX ||
      padding_end_u64 > (uint64_t)SIZE_MAX ||
      crc_end_u64 > (uint64_t)SIZE_MAX) {
    tide_set_parse_error(error, TIDE_STATUS_RESOURCE, (uint64_t)offset, frame->type, depth, "record size overflow");
    return TIDE_STATUS_RESOURCE;
  }
  frame->payload_end = (size_t)payload_end_u64;
  frame->padding_end = (size_t)padding_end_u64;
  frame->crc_end = (size_t)crc_end_u64;
  frame->depth = depth;
  if (frame->crc_end > size) {
    return TIDE_STATUS_TRUNCATED;
  }
  return TIDE_STATUS_OK;
}

static tide_status tide_validate_record_integrity(const uint8_t *data,
                                                  const tide_record_frame *frame,
                                                  tide_error *error) {
  size_t i;
  uint32_t stored;
  uint32_t actual;
  for (i = frame->payload_end; i < frame->padding_end; ++i) {
    if (data[i] != 0u) {
      tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)i, frame->type, frame->depth, "nonzero padding");
      return TIDE_STATUS_FORMAT;
    }
  }
  stored = (uint32_t)data[frame->padding_end] |
           ((uint32_t)data[frame->padding_end + 1u] << 8u) |
           ((uint32_t)data[frame->padding_end + 2u] << 16u) |
           ((uint32_t)data[frame->padding_end + 3u] << 24u);
  actual = tide_crc32c(data + frame->offset, frame->padding_end - frame->offset);
  if (stored != actual) {
    tide_set_parse_error(error, TIDE_STATUS_INTEGRITY, (uint64_t)frame->padding_end, frame->type, frame->depth, "record crc mismatch");
    return TIDE_STATUS_INTEGRITY;
  }
  return TIDE_STATUS_OK;
}

static tide_status tide_parse_records(tide_decoder_impl *impl,
                                      const uint8_t *data,
                                      size_t begin,
                                      size_t end,
                                      uint8_t depth,
                                      tide_error *error,
                                      uint64_t *valid_prefix);

static tide_status tide_emit_stream(tide_decoder_impl *impl,
                                    const uint8_t *payload,
                                    size_t payload_size,
                                    const tide_record_frame *frame,
                                    tide_error *error) {
  tide_reader reader;
  tide_stream_info stream;
  uint64_t config_size_u64;
  size_t config_size;
  tide_reader_init(&reader, payload, payload_size);
  memset(&stream, 0, sizeof(stream));
  if (!tide_reader_read_u32(&reader, &stream.track_id) ||
      !tide_reader_read_u32(&reader, &stream.generation) ||
      !tide_reader_read_u16(&reader, &stream.media_kind) ||
      !tide_reader_skip(&reader, 2u) ||
      !tide_reader_read_u32(&reader, &stream.codec_tag) ||
      !tide_reader_read_u32(&reader, &stream.time_base.numerator) ||
      !tide_reader_read_u32(&reader, &stream.time_base.denominator) ||
      !tide_reader_read_uleb128(&reader, &config_size_u64) ||
      !tide_checked_u64_to_size(config_size_u64, &config_size) ||
      !tide_reader_read_bytes(&reader, &stream.config, config_size) ||
      reader.offset != payload_size) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad stream descriptor");
    return TIDE_STATUS_FORMAT;
  }
  stream.config_size = config_size;
  if (!tide_rational_is_valid(stream.time_base)) {
    tide_set_parse_error(error, TIDE_STATUS_TIMELINE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "invalid stream time base");
    return TIDE_STATUS_TIMELINE;
  }
  if (tide_add_track(impl, &stream, error, (uint64_t)frame->offset) != TIDE_STATUS_OK) {
    return error->code;
  }
  if (impl->callbacks.on_stream != NULL) {
    return impl->callbacks.on_stream(impl->user, &stream);
  }
  return TIDE_STATUS_OK;
}

static tide_status tide_emit_packet(tide_decoder_impl *impl,
                                    const uint8_t *payload,
                                    size_t payload_size,
                                    const tide_record_frame *frame,
                                    tide_error *error) {
  tide_reader reader;
  tide_packet_info packet;
  uint64_t packet_payload_u64;
  size_t packet_payload_size;
  tide_reader_init(&reader, payload, payload_size);
  memset(&packet, 0, sizeof(packet));
  if (!tide_reader_read_u32(&reader, &packet.track_id) ||
      !tide_reader_read_u32(&reader, &packet.generation) ||
      !tide_reader_read_u64(&reader, &packet.packet_seq) ||
      !tide_reader_read_i64(&reader, &packet.dts) ||
      !tide_reader_read_i64(&reader, &packet.pts) ||
      !tide_reader_read_i64(&reader, &packet.duration) ||
      !tide_reader_read_u32(&reader, &packet.flags) ||
      !tide_reader_read_uleb128(&reader, &packet_payload_u64) ||
      !tide_checked_u64_to_size(packet_payload_u64, &packet_payload_size) ||
      !tide_reader_read_bytes(&reader, &packet.payload.data, packet_payload_size) ||
      reader.offset != payload_size) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad packet");
    return TIDE_STATUS_FORMAT;
  }
  packet.payload.size = packet_payload_size;
  if (packet.duration < 0) {
    tide_set_parse_error(error, TIDE_STATUS_TIMELINE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "negative packet duration");
    return TIDE_STATUS_TIMELINE;
  }
  if (tide_find_track(impl, packet.track_id, packet.generation) == NULL) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "packet before descriptor");
    return TIDE_STATUS_FORMAT;
  }
  if (impl->callbacks.on_packet != NULL) {
    return impl->callbacks.on_packet(impl->user, &packet);
  }
  return TIDE_STATUS_OK;
}

static tide_status tide_emit_simple_record(tide_decoder_impl *impl,
                                           const uint8_t *payload,
                                           size_t payload_size,
                                           const tide_record_frame *frame,
                                           tide_error *error) {
  tide_reader reader;
  uint64_t count;
  tide_reader_init(&reader, payload, payload_size);
  switch (frame->type) {
    case TIDE_RECORD_PACKET_TABLE: {
      uint32_t track_id;
      uint32_t generation;
      uint64_t previous_end = 0u;
      uint64_t previous_seq = 0u;
      uint64_t i;
      if (!tide_reader_read_u32(&reader, &track_id) ||
          !tide_reader_read_u32(&reader, &generation) ||
          !tide_reader_read_uleb128(&reader, &count) ||
          count > impl->limits.max_packet_table_entries ||
          tide_find_track(impl, track_id, generation) == NULL) {
        tide_set_parse_error(error, TIDE_STATUS_RESOURCE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "packet table limit");
        return TIDE_STATUS_RESOURCE;
      }
      for (i = 0u; i < count; ++i) {
        uint64_t packet_seq;
        int64_t dts;
        int64_t pts;
        int64_t duration;
        uint32_t flags;
        uint64_t range_offset;
        uint64_t range_size;
        uint64_t range_end;
        (void)dts;
        (void)pts;
        (void)flags;
        if (!tide_reader_read_u64(&reader, &packet_seq) ||
            !tide_reader_read_i64(&reader, &dts) ||
            !tide_reader_read_i64(&reader, &pts) ||
            !tide_reader_read_i64(&reader, &duration) ||
            !tide_reader_read_u32(&reader, &flags) ||
            !tide_reader_read_u64(&reader, &range_offset) ||
            !tide_reader_read_u64(&reader, &range_size) ||
            duration < 0 ||
            !tide_checked_add_u64(range_offset, range_size, &range_end) ||
            (i != 0u && packet_seq <= previous_seq) ||
            range_offset < previous_end) {
          tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad packet table entry");
          return TIDE_STATUS_FORMAT;
        }
        previous_seq = packet_seq;
        previous_end = range_end;
      }
      if (reader.offset != payload_size) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "packet table trailing bytes");
        return TIDE_STATUS_FORMAT;
      }
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_EDIT_LIST: {
      uint32_t track_id;
      uint32_t generation;
      int64_t previous_end = INT64_MIN;
      uint64_t i;
      if (!tide_reader_read_u32(&reader, &track_id) ||
          !tide_reader_read_u32(&reader, &generation) ||
          !tide_reader_read_uleb128(&reader, &count) ||
          count > impl->limits.max_edit_entries ||
          tide_find_track(impl, track_id, generation) == NULL) {
        tide_set_parse_error(error, TIDE_STATUS_TIMELINE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad edit list");
        return TIDE_STATUS_TIMELINE;
      }
      for (i = 0u; i < count; ++i) {
        int64_t output_start;
        int64_t output_duration;
        int64_t source_start;
        tide_rational rate;
        int64_t output_end;
        if (!tide_reader_read_i64(&reader, &output_start) ||
            !tide_reader_read_i64(&reader, &output_duration) ||
            !tide_reader_read_i64(&reader, &source_start) ||
            !tide_reader_read_u32(&reader, &rate.numerator) ||
            !tide_reader_read_u32(&reader, &rate.denominator) ||
            output_duration < 0 ||
            output_start > INT64_MAX - output_duration ||
            !tide_rational_is_valid(rate)) {
          tide_set_parse_error(error, TIDE_STATUS_TIMELINE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad edit entry");
          return TIDE_STATUS_TIMELINE;
        }
        output_end = output_start + output_duration;
        if (i != 0u && output_start < previous_end) {
          tide_set_parse_error(error, TIDE_STATUS_TIMELINE, (uint64_t)frame->payload_offset, frame->type, frame->depth, "overlapping edit entries");
          return TIDE_STATUS_TIMELINE;
        }
        (void)source_start;
        previous_end = output_end;
      }
      if (reader.offset != payload_size) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "edit list trailing bytes");
        return TIDE_STATUS_FORMAT;
      }
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_DISCONTINUITY: {
      uint32_t track_id;
      uint32_t generation;
      uint64_t epoch;
      uint32_t reason;
      if (!tide_reader_read_u32(&reader, &track_id) ||
          !tide_reader_read_u32(&reader, &generation) ||
          !tide_reader_read_u64(&reader, &epoch) ||
          !tide_reader_read_u32(&reader, &reason) ||
          epoch == 0u ||
          reader.offset != payload_size ||
          (track_id != 0u && tide_find_track(impl, track_id, generation) == NULL)) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad discontinuity");
        return TIDE_STATUS_FORMAT;
      }
      (void)reason;
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_SEEK_INDEX: {
      uint64_t index_generation;
      uint64_t previous_sort_key = 0u;
      uint64_t i;
      if (!tide_reader_read_u64(&reader, &index_generation) ||
          !tide_reader_read_uleb128(&reader, &count)) {
        tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad seek index header");
        return TIDE_STATUS_INDEX;
      }
      (void)index_generation;
      for (i = 0u; i < count; ++i) {
        uint32_t track_id;
        uint32_t generation;
        uint64_t packet_seq;
        int64_t pts;
        uint64_t group_offset;
        uint64_t record_offset;
        uint64_t sort_key;
        if (!tide_reader_read_u32(&reader, &track_id) ||
            !tide_reader_read_u32(&reader, &generation) ||
            !tide_reader_read_u64(&reader, &packet_seq) ||
            !tide_reader_read_i64(&reader, &pts) ||
            !tide_reader_read_u64(&reader, &group_offset) ||
            !tide_reader_read_u64(&reader, &record_offset) ||
            tide_find_track(impl, track_id, generation) == NULL ||
            group_offset < TIDE_HEADER_SIZE ||
            record_offset < group_offset ||
            record_offset >= (uint64_t)frame->offset) {
          tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad seek index entry");
          return TIDE_STATUS_INDEX;
        }
        sort_key = ((uint64_t)track_id << 32u) ^ (uint64_t)(pts < 0 ? 0 : pts);
        if (i != 0u && sort_key < previous_sort_key) {
          tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "unsorted seek index");
          return TIDE_STATUS_INDEX;
        }
        previous_sort_key = sort_key;
        (void)packet_seq;
      }
      if (reader.offset != payload_size) {
        tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "seek index trailing bytes");
        return TIDE_STATUS_INDEX;
      }
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_CHECKPOINT: {
      uint64_t group_sequence;
      uint64_t prefix_offset;
      const uint8_t *digest;
      uint64_t i;
      if (!tide_reader_read_u64(&reader, &group_sequence) ||
          !tide_reader_read_u64(&reader, &prefix_offset) ||
          !tide_reader_read_bytes(&reader, &digest, TIDE_DIGEST_SIZE) ||
          !tide_reader_read_uleb128(&reader, &count) ||
          prefix_offset > (uint64_t)frame->offset ||
          count > impl->limits.max_tracks) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad checkpoint");
        return TIDE_STATUS_FORMAT;
      }
      for (i = 0u; i < count; ++i) {
        uint32_t track_id;
        uint32_t generation;
        if (!tide_reader_read_u32(&reader, &track_id) ||
            !tide_reader_read_u32(&reader, &generation) ||
            tide_find_track(impl, track_id, generation) == NULL) {
          tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad checkpoint descriptor");
          return TIDE_STATUS_FORMAT;
        }
      }
      if (reader.offset != payload_size) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "checkpoint trailing bytes");
        return TIDE_STATUS_FORMAT;
      }
      (void)group_sequence;
      (void)digest;
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_INDEX_DIRECTORY: {
      uint64_t previous_end = 0u;
      uint64_t previous_generation = 0u;
      uint64_t i;
      if (!tide_reader_read_uleb128(&reader, &count)) {
        tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad index directory");
        return TIDE_STATUS_INDEX;
      }
      for (i = 0u; i < count; ++i) {
        uint64_t generation;
        int64_t start_time;
        int64_t end_time;
        uint64_t offset;
        if (!tide_reader_read_u64(&reader, &generation) ||
            !tide_reader_read_i64(&reader, &start_time) ||
            !tide_reader_read_i64(&reader, &end_time) ||
            !tide_reader_read_u64(&reader, &offset) ||
            start_time < 0 ||
            end_time < start_time ||
            offset < TIDE_HEADER_SIZE ||
            offset >= (uint64_t)frame->offset ||
            (i != 0u && generation <= previous_generation) ||
            (i != 0u && (uint64_t)start_time < previous_end)) {
          tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad index directory entry");
          return TIDE_STATUS_INDEX;
        }
        previous_generation = generation;
        previous_end = (uint64_t)end_time;
      }
      if (reader.offset != payload_size) {
        tide_set_parse_error(error, TIDE_STATUS_INDEX, (uint64_t)frame->payload_offset, frame->type, frame->depth, "index directory trailing bytes");
        return TIDE_STATUS_INDEX;
      }
      return TIDE_STATUS_OK;
    }
    case TIDE_RECORD_FOOTER: {
      uint64_t file_length;
      uint64_t index_directory_offset;
      uint64_t last_checkpoint_sequence;
      const uint8_t *digest;
      if (!tide_reader_read_u64(&reader, &file_length) ||
          !tide_reader_read_u64(&reader, &index_directory_offset) ||
          !tide_reader_read_uleb128(&reader, &last_checkpoint_sequence) ||
          !tide_reader_read_bytes(&reader, &digest, TIDE_DIGEST_SIZE) ||
          reader.offset != payload_size ||
          file_length != (uint64_t)frame->crc_end ||
          (index_directory_offset != 0u &&
           (index_directory_offset < TIDE_HEADER_SIZE || index_directory_offset >= (uint64_t)frame->offset))) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad footer");
        return TIDE_STATUS_FORMAT;
      }
      (void)last_checkpoint_sequence;
      (void)digest;
      return TIDE_STATUS_OK;
    }
    default:
      return TIDE_STATUS_OK;
  }
}

static tide_status tide_process_record(tide_decoder_impl *impl,
                                       const uint8_t *data,
                                       const tide_record_frame *frame,
                                       tide_error *error,
                                       uint64_t *valid_prefix) {
  tide_record_event event;
  const uint8_t *payload = data + frame->payload_offset;
  size_t payload_size = frame->payload_end - frame->payload_offset;
  tide_status status;

  memset(&event, 0, sizeof(event));
  event.type = frame->type;
  event.flags = frame->flags;
  event.offset = (uint64_t)frame->offset;
  event.payload_offset = (uint64_t)frame->payload_offset;
  event.payload_size = frame->payload_size;
  event.sequence = frame->sequence;
  event.depth = frame->depth;
  event.skippable = (frame->type & TIDE_RECORD_SKIPPABLE_BIT) != 0u;

  if ((frame->type & TIDE_RECORD_SKIPPABLE_BIT) != 0u) {
    if (impl->callbacks.on_record != NULL) {
      return impl->callbacks.on_record(impl->user, &event);
    }
    return TIDE_STATUS_OK;
  }
  if (impl->callbacks.on_record != NULL) {
    status = impl->callbacks.on_record(impl->user, &event);
    if (status != TIDE_STATUS_OK) {
      return status;
    }
  }

  switch (frame->type) {
    case TIDE_RECORD_GROUP: {
      tide_reader reader;
      uint16_t group_kind;
      uint64_t child_bytes;
      uint64_t group_sequence;
      size_t child_begin;
      size_t child_end;
      (void)group_kind;
      (void)group_sequence;
      if (payload_size < TIDE_GROUP_META_SIZE) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "short group");
        return TIDE_STATUS_FORMAT;
      }
      tide_reader_init(&reader, payload, payload_size);
      if (!tide_reader_read_u16(&reader, &group_kind) ||
          !tide_reader_skip(&reader, 2u) ||
          !tide_reader_read_u64(&reader, &child_bytes) ||
          !tide_reader_read_u64(&reader, &group_sequence) ||
          child_bytes != (uint64_t)(payload_size - TIDE_GROUP_META_SIZE)) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)frame->payload_offset, frame->type, frame->depth, "bad group child length");
        return TIDE_STATUS_FORMAT;
      }
      if (frame->depth >= impl->limits.max_depth) {
        tide_set_parse_error(error, TIDE_STATUS_RESOURCE, (uint64_t)frame->offset, frame->type, frame->depth, "group depth limit");
        return TIDE_STATUS_RESOURCE;
      }
      child_begin = frame->payload_offset + TIDE_GROUP_META_SIZE;
      child_end = frame->payload_end;
      status = tide_parse_records(impl, data, child_begin, child_end, (uint8_t)(frame->depth + 1u), error, valid_prefix);
      if (status == TIDE_STATUS_TRUNCATED) {
        tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)child_begin, frame->type, frame->depth, "complete group contains incomplete child");
        return TIDE_STATUS_FORMAT;
      }
      return status;
    }
    case TIDE_RECORD_STREAM_DESC:
      return tide_emit_stream(impl, payload, payload_size, frame, error);
    case TIDE_RECORD_PACKET:
      return tide_emit_packet(impl, payload, payload_size, frame, error);
    case TIDE_RECORD_PACKET_TABLE:
    case TIDE_RECORD_EDIT_LIST:
    case TIDE_RECORD_DISCONTINUITY:
    case TIDE_RECORD_SEEK_INDEX:
    case TIDE_RECORD_CHECKPOINT:
    case TIDE_RECORD_INDEX_DIRECTORY:
    case TIDE_RECORD_FOOTER:
      return tide_emit_simple_record(impl, payload, payload_size, frame, error);
    default:
      tide_set_parse_error(error, TIDE_STATUS_UNSUPPORTED, (uint64_t)frame->offset, frame->type, frame->depth, "unknown required record");
      return TIDE_STATUS_UNSUPPORTED;
  }
}

static tide_status tide_parse_records(tide_decoder_impl *impl,
                                      const uint8_t *data,
                                      size_t begin,
                                      size_t end,
                                      uint8_t depth,
                                      tide_error *error,
                                      uint64_t *valid_prefix) {
  size_t cursor = begin;
  while (cursor < end) {
    tide_record_frame frame;
    tide_status status = tide_decode_record_prefix(data, end, cursor, depth, &frame, error);
    if (status == TIDE_STATUS_TRUNCATED) {
      *valid_prefix = (uint64_t)cursor;
      return TIDE_STATUS_TRUNCATED;
    }
    if (status != TIDE_STATUS_OK) {
      return status;
    }
    if (frame.payload_size > impl->limits.max_payload_size ||
        (uint64_t)(frame.crc_end - frame.offset) > impl->limits.max_record_size) {
      tide_set_parse_error(error, TIDE_STATUS_RESOURCE, (uint64_t)cursor, frame.type, depth, "record exceeds limit");
      return TIDE_STATUS_RESOURCE;
    }
    status = tide_validate_record_integrity(data, &frame, error);
    if (status != TIDE_STATUS_OK) {
      return status;
    }
    status = tide_process_record(impl, data, &frame, error, valid_prefix);
    if (status != TIDE_STATUS_OK) {
      return status;
    }
    cursor = frame.crc_end;
    *valid_prefix = (uint64_t)cursor;
  }
  if (cursor != end) {
    tide_set_parse_error(error, TIDE_STATUS_FORMAT, (uint64_t)cursor, 0, depth, "record boundary mismatch");
    return TIDE_STATUS_FORMAT;
  }
  return TIDE_STATUS_OK;
}

tide_status tide_parse_tide_bytes(const uint8_t *data,
                                  size_t size,
                                  int end,
                                  const tide_limits *limits,
                                  const tide_callbacks *callbacks,
                                  void *user,
                                  tide_error *error,
                                  uint64_t *valid_prefix) {
  tide_decoder_impl impl;
  tide_status status;
  tide_limits effective_limits = limits == NULL ? tide_limits_default() : *limits;

  if (data == NULL && size != 0u) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  memset(&impl, 0, sizeof(impl));
  impl.limits = effective_limits;
  if (callbacks != NULL) {
    impl.callbacks = *callbacks;
  }
  impl.user = user;
  tide_error_clear(error);
  if (valid_prefix != NULL) {
    *valid_prefix = 0u;
  }
  status = tide_validate_header(data, size, &impl.limits, error);
  if (status != TIDE_STATUS_OK) {
    free(impl.tracks);
    return status == TIDE_STATUS_TRUNCATED && end ? TIDE_STATUS_PARTIAL : status;
  }
  if (valid_prefix != NULL) {
    *valid_prefix = TIDE_HEADER_SIZE;
  }
  status = tide_parse_records(&impl, data, TIDE_HEADER_SIZE, size, 0u, error, valid_prefix);
  free(impl.tracks);
  if (status == TIDE_STATUS_TRUNCATED && end) {
    return TIDE_STATUS_PARTIAL;
  }
  return status;
}

tide_status tide_decoder_init(tide_decoder *decoder,
                              const tide_limits *limits,
                              const tide_callbacks *callbacks,
                              void *user) {
  tide_decoder_impl *impl;
  if (decoder == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  decoder->impl = NULL;
  impl = (tide_decoder_impl *)calloc(1u, sizeof(*impl));
  if (impl == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  impl->limits = limits == NULL ? tide_limits_default() : *limits;
  if (callbacks != NULL) {
    impl->callbacks = *callbacks;
  }
  impl->user = user;
  impl->buffer.aggregate_limit = impl->limits.max_aggregate_alloc;
  decoder->impl = impl;
  return TIDE_STATUS_OK;
}

tide_status tide_decoder_feed(tide_decoder *decoder,
                              const uint8_t *data,
                              size_t size,
                              int end,
                              size_t *consumed) {
  tide_decoder_impl *impl = tide_impl(decoder);
  tide_status status;
  if (consumed != NULL) {
    *consumed = 0u;
  }
  if (impl == NULL || (data == NULL && size != 0u)) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (impl->cancelled) {
    return TIDE_STATUS_CANCELLED;
  }
  if (impl->terminal) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  status = tide_buffer_append(&impl->buffer, data, size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  if (consumed != NULL) {
    *consumed = size;
  }
  if (!end) {
    return TIDE_STATUS_OK;
  }
  status = tide_parse_tide_bytes(impl->buffer.data,
                                 impl->buffer.size,
                                 1,
                                 &impl->limits,
                                 &impl->callbacks,
                                 impl->user,
                                 &impl->error,
                                 &impl->valid_prefix);
  if (status != TIDE_STATUS_OK && status != TIDE_STATUS_PARTIAL &&
      impl->callbacks.on_error != NULL) {
    impl->callbacks.on_error(impl->user, &impl->error);
  }
  impl->terminal = 1;
  return status;
}

void tide_decoder_cancel(tide_decoder *decoder) {
  tide_decoder_impl *impl = tide_impl(decoder);
  if (impl != NULL) {
    impl->cancelled = 1;
  }
}

void tide_decoder_destroy(tide_decoder *decoder) {
  tide_decoder_impl *impl = tide_impl(decoder);
  if (impl != NULL) {
    tide_buffer_destroy(&impl->buffer);
    free(impl->tracks);
    free(impl);
    decoder->impl = NULL;
  }
}

const tide_error *tide_decoder_last_error(const tide_decoder *decoder) {
  const tide_decoder_impl *impl = decoder == NULL ? NULL : (const tide_decoder_impl *)decoder->impl;
  return impl == NULL ? NULL : &impl->error;
}

uint64_t tide_decoder_valid_prefix(const tide_decoder *decoder) {
  const tide_decoder_impl *impl = decoder == NULL ? NULL : (const tide_decoder_impl *)decoder->impl;
  return impl == NULL ? 0u : impl->valid_prefix;
}
