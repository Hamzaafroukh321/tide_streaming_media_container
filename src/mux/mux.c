#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

struct tide_mux {
  FILE *file;
  tide_buffer buffer;
  tide_limits limits;
  tide_rational movie_time_base;
  uint64_t sequence;
  int closed;
  int write_footer;
  tide_error error;
};

static void tide_mux_set_error(struct tide_mux *mux, tide_status status, const char *message) {
  if (mux != NULL) {
    tide_error_set(&mux->error, status, TIDE_SEVERITY_ERROR, mux->buffer.size, 0, 0, "mux", message);
  }
}

tide_status tide_mux_create(tide_mux **out,
                            const char *path,
                            const tide_mux_options *options) {
  struct tide_mux *mux;
  uint8_t uuid[TIDE_UUID_SIZE] = {
    0x54u, 0x49u, 0x44u, 0x45u, 0x2du, 0x30u, 0x30u, 0x31u,
    0x2du, 0x6du, 0x75u, 0x78u, 0x2du, 0x30u, 0x31u, 0x00u
  };
  tide_status status;
  if (out == NULL || path == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = NULL;
  mux = (struct tide_mux *)calloc(1u, sizeof(*mux));
  if (mux == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  mux->limits = (options == NULL || options->limits.max_record_size == 0u) ? tide_limits_default() : options->limits;
  mux->movie_time_base = (options == NULL || options->movie_time_base.numerator == 0u) ? (tide_rational){1u, 1000u} : options->movie_time_base;
  mux->write_footer = options == NULL ? 1 : options->write_footer;
  mux->buffer.aggregate_limit = mux->limits.max_aggregate_alloc;
  mux->file = fopen(path, "wb");
  if (mux->file == NULL) {
    free(mux);
    return TIDE_STATUS_IO;
  }
  status = tide_write_header(&mux->buffer, mux->movie_time_base, uuid);
  if (status != TIDE_STATUS_OK) {
    tide_mux_abort(mux);
    return status;
  }
  *out = mux;
  return TIDE_STATUS_OK;
}

tide_status tide_mux_add_stream(tide_mux *opaque, const tide_stream_info *stream) {
  struct tide_mux *mux = (struct tide_mux *)opaque;
  tide_buffer payload = {0};
  tide_status status;
  if (mux == NULL || stream == NULL || mux->closed) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  payload.aggregate_limit = mux->limits.max_single_alloc;
  status = tide_encode_stream_payload(&payload, stream);
  if (status == TIDE_STATUS_OK) {
    status = tide_write_record(&mux->buffer, TIDE_RECORD_STREAM_DESC, 0u, 3u, mux->sequence++, payload.data, payload.size);
  }
  tide_buffer_destroy(&payload);
  if (status != TIDE_STATUS_OK) {
    tide_mux_set_error(mux, status, "failed to write stream");
  }
  return status;
}

tide_status tide_mux_write_packet(tide_mux *opaque, tide_packet_ref *moved_packet) {
  struct tide_mux *mux = (struct tide_mux *)opaque;
  tide_buffer payload = {0};
  tide_status status;
  if (mux == NULL || moved_packet == NULL || mux->closed) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  payload.aggregate_limit = mux->limits.max_single_alloc;
  status = tide_encode_packet_payload(&payload, &moved_packet->info);
  if (status == TIDE_STATUS_OK) {
    status = tide_write_record(&mux->buffer, TIDE_RECORD_PACKET, 0u, 3u, mux->sequence++, payload.data, payload.size);
  }
  tide_buffer_destroy(&payload);
  tide_packet_ref_reset(moved_packet);
  if (status != TIDE_STATUS_OK) {
    tide_mux_set_error(mux, status, "failed to write packet");
  }
  return status;
}

static tide_status tide_mux_append_footer(struct tide_mux *mux) {
  uint8_t payload[49];
  tide_buffer record = {0};
  tide_status status;
  uint64_t final_length;
  memset(payload, 0, sizeof(payload));
  record.aggregate_limit = mux->limits.max_single_alloc;
  status = tide_write_record(&record, TIDE_RECORD_FOOTER, 0u, 3u, mux->sequence, payload, sizeof(payload));
  if (status != TIDE_STATUS_OK) {
    tide_buffer_destroy(&record);
    return status;
  }
  final_length = (uint64_t)mux->buffer.size + (uint64_t)record.size;
  tide_buffer_destroy(&record);
  record.data = NULL;
  record.size = 0;
  record.capacity = 0;
  record.aggregate_limit = mux->limits.max_single_alloc;
  tide_write_u64(payload, final_length);
  tide_write_u64(payload + 8u, 0u);
  payload[16] = 0u;
  status = tide_write_record(&record, TIDE_RECORD_FOOTER, 0u, 3u, mux->sequence++, payload, sizeof(payload));
  if (status == TIDE_STATUS_OK) {
    status = tide_buffer_append(&mux->buffer, record.data, record.size);
  }
  tide_buffer_destroy(&record);
  return status;
}

tide_status tide_mux_close(tide_mux *opaque) {
  struct tide_mux *mux = (struct tide_mux *)opaque;
  tide_status status = TIDE_STATUS_OK;
  if (mux == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (!mux->closed && mux->write_footer) {
    status = tide_mux_append_footer(mux);
  }
  if (status == TIDE_STATUS_OK &&
      fwrite(mux->buffer.data, 1u, mux->buffer.size, mux->file) != mux->buffer.size) {
    status = TIDE_STATUS_IO;
  }
  if (fclose(mux->file) != 0 && status == TIDE_STATUS_OK) {
    status = TIDE_STATUS_IO;
  }
  mux->file = NULL;
  mux->closed = 1;
  tide_buffer_destroy(&mux->buffer);
  free(mux);
  return status;
}

void tide_mux_abort(tide_mux *opaque) {
  struct tide_mux *mux = (struct tide_mux *)opaque;
  if (mux != NULL) {
    if (mux->file != NULL) {
      fclose(mux->file);
    }
    tide_buffer_destroy(&mux->buffer);
    free(mux);
  }
}

const tide_error *tide_mux_last_error(const tide_mux *opaque) {
  const struct tide_mux *mux = (const struct tide_mux *)opaque;
  return mux == NULL ? NULL : &mux->error;
}
