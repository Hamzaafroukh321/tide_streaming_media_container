#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

struct tide_demux {
  tide_source *source;
  tide_error error;
  tide_stream_info *streams;
  size_t stream_count;
  size_t stream_capacity;
  uint8_t **configs;
  tide_packet_ref *packets;
  size_t packet_count;
  size_t packet_capacity;
  size_t cursor;
  uint32_t selected_track;
};

static tide_status tide_demux_add_stream(struct tide_demux *demux,
                                         const tide_stream_info *stream) {
  tide_stream_info *grown_streams;
  uint8_t **grown_configs;
  uint8_t *config_copy = NULL;
  if (demux->stream_count == demux->stream_capacity) {
    size_t next = demux->stream_capacity == 0u ? 8u : demux->stream_capacity * 2u;
    grown_streams = (tide_stream_info *)realloc(demux->streams, next * sizeof(*grown_streams));
    if (grown_streams == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    demux->streams = grown_streams;
    grown_configs = (uint8_t **)realloc(demux->configs, next * sizeof(*grown_configs));
    if (grown_configs == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    demux->configs = grown_configs;
    demux->stream_capacity = next;
  }
  if (stream->config_size != 0u) {
    config_copy = (uint8_t *)malloc(stream->config_size);
    if (config_copy == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    memcpy(config_copy, stream->config, stream->config_size);
  }
  demux->streams[demux->stream_count] = *stream;
  demux->streams[demux->stream_count].config = config_copy;
  demux->configs[demux->stream_count] = config_copy;
  demux->stream_count += 1u;
  return TIDE_STATUS_OK;
}

static tide_status tide_demux_add_packet(struct tide_demux *demux,
                                         const tide_packet_info *packet) {
  tide_packet_ref *grown;
  tide_payload_lease *lease;
  if (demux->selected_track != 0u && packet->track_id != demux->selected_track) {
    return TIDE_STATUS_OK;
  }
  if (demux->packet_count == demux->packet_capacity) {
    size_t next = demux->packet_capacity == 0u ? 16u : demux->packet_capacity * 2u;
    grown = (tide_packet_ref *)realloc(demux->packets, next * sizeof(*grown));
    if (grown == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    demux->packets = grown;
    while (demux->packet_capacity < next) {
      tide_packet_ref_init(&demux->packets[demux->packet_capacity]);
      demux->packet_capacity += 1u;
    }
  }
  lease = tide_payload_lease_create_copy(packet->payload.data, packet->payload.size);
  if (lease == NULL && packet->payload.size != 0u) {
    return TIDE_STATUS_RESOURCE;
  }
  demux->packets[demux->packet_count].info = *packet;
  demux->packets[demux->packet_count].lease = lease;
  demux->packets[demux->packet_count].info.payload.lease = lease;
  demux->packets[demux->packet_count].info.payload.data = tide_payload_lease_data(lease);
  demux->packets[demux->packet_count].info.payload.size = tide_payload_lease_size(lease);
  demux->packet_count += 1u;
  return TIDE_STATUS_OK;
}

static tide_status on_demux_stream(void *user, const tide_stream_info *stream) {
  return tide_demux_add_stream((struct tide_demux *)user, stream);
}

static tide_status on_demux_packet(void *user, const tide_packet_info *packet) {
  return tide_demux_add_packet((struct tide_demux *)user, packet);
}

static void on_demux_error(void *user, const tide_error *error) {
  struct tide_demux *demux = (struct tide_demux *)user;
  if (demux != NULL && error != NULL) {
    demux->error = *error;
  }
}

tide_status tide_demux_open(tide_demux **out,
                            tide_source *source,
                            const tide_demux_options *options) {
  struct tide_demux *demux;
  tide_callbacks callbacks;
  tide_status status;
  uint64_t valid_prefix = 0;
  tide_limits limits;
  if (out == NULL || source == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = NULL;
  demux = (struct tide_demux *)calloc(1u, sizeof(*demux));
  if (demux == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  demux->source = source;
  limits = (options == NULL || options->limits.max_record_size == 0u) ? tide_limits_default() : options->limits;
  demux->selected_track = options == NULL ? 0u : options->selected_track;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.size = sizeof(callbacks);
  callbacks.on_stream = on_demux_stream;
  callbacks.on_packet = on_demux_packet;
  callbacks.on_error = on_demux_error;
  status = tide_parse_tide_bytes(tide_source_data(source),
                                 tide_source_size(source),
                                 1,
                                 &limits,
                                 &callbacks,
                                 demux,
                                 &demux->error,
                                 &valid_prefix);
  if (status != TIDE_STATUS_OK && !(status == TIDE_STATUS_PARTIAL && options != NULL && !options->strict)) {
    tide_demux_close(demux);
    return status;
  }
  *out = demux;
  return status;
}

tide_status tide_demux_next(tide_demux *demux, tide_packet_ref *out) {
  if (demux == NULL || out == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (demux->cursor >= demux->packet_count) {
    return TIDE_STATUS_WOULD_BLOCK;
  }
  return tide_packet_ref_move(out, &demux->packets[demux->cursor++]);
}

size_t tide_demux_stream_count(const tide_demux *demux) {
  return demux == NULL ? 0u : demux->stream_count;
}

tide_status tide_demux_stream_info(const tide_demux *demux,
                                   size_t index,
                                   tide_stream_info *out) {
  if (demux == NULL || out == NULL || index >= demux->stream_count) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = demux->streams[index];
  return TIDE_STATUS_OK;
}

const tide_error *tide_demux_last_error(const tide_demux *demux) {
  return demux == NULL ? NULL : &demux->error;
}

void tide_demux_close(tide_demux *opaque) {
  struct tide_demux *demux = (struct tide_demux *)opaque;
  if (demux != NULL) {
    size_t i;
    for (i = 0; i < demux->packet_capacity; ++i) {
      tide_packet_ref_reset(&demux->packets[i]);
    }
    for (i = 0; i < demux->stream_count; ++i) {
      free(demux->configs[i]);
    }
    free(demux->configs);
    free(demux->streams);
    free(demux->packets);
    free(demux);
  }
}
