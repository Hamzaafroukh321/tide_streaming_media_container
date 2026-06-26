#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

struct tide_repair_plan {
  tide_source *source;
  uint64_t valid_prefix;
  tide_error error;
};

tide_status tide_repair_scan(tide_source *source,
                             const tide_repair_options *options,
                             tide_repair_plan **out) {
  struct tide_repair_plan *plan;
  tide_status status;
  tide_limits limits;
  if (out == NULL || source == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = NULL;
  plan = (struct tide_repair_plan *)calloc(1u, sizeof(*plan));
  if (plan == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  plan->source = source;
  limits = (options == NULL || options->limits.max_record_size == 0u) ? tide_limits_default() : options->limits;
  status = tide_parse_tide_bytes(tide_source_data(source),
                                 tide_source_size(source),
                                 1,
                                 &limits,
                                 NULL,
                                 NULL,
                                 &plan->error,
                                 &plan->valid_prefix);
  if (status == TIDE_STATUS_OK || status == TIDE_STATUS_PARTIAL) {
    *out = plan;
    return status;
  }
  *out = plan;
  return status;
}

tide_status tide_repair_write(tide_repair_plan *opaque, const char *path) {
  struct tide_repair_plan *plan = (struct tide_repair_plan *)opaque;
  tide_source *prefix_source = NULL;
  tide_demux *demux = NULL;
  tide_mux *mux = NULL;
  tide_demux_options demux_options;
  tide_status status;
  size_t i;
  if (plan == NULL || path == NULL || plan->valid_prefix == 0u) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  status = tide_source_from_memory(&prefix_source,
                                   tide_source_data(plan->source),
                                   (size_t)plan->valid_prefix);
  if (status != TIDE_STATUS_OK) {
    goto cleanup;
  }
  memset(&demux_options, 0, sizeof(demux_options));
  demux_options.size = sizeof(demux_options);
  demux_options.limits = tide_limits_default();
  demux_options.strict = 0;
  status = tide_demux_open(&demux, prefix_source, &demux_options);
  if (status == TIDE_STATUS_PARTIAL) {
    status = TIDE_STATUS_OK;
  }
  if (status != TIDE_STATUS_OK) {
    goto cleanup;
  }
  status = tide_mux_create(&mux, path, NULL);
  if (status != TIDE_STATUS_OK) {
    goto cleanup;
  }
  for (i = 0u; i < tide_demux_stream_count(demux); ++i) {
    tide_stream_info stream;
    status = tide_demux_stream_info(demux, i, &stream);
    if (status == TIDE_STATUS_OK) {
      status = tide_mux_add_stream(mux, &stream);
    }
    if (status != TIDE_STATUS_OK) {
      goto cleanup;
    }
  }
  for (;;) {
    tide_packet_ref packet;
    tide_packet_ref_init(&packet);
    status = tide_demux_next(demux, &packet);
    if (status == TIDE_STATUS_WOULD_BLOCK) {
      status = TIDE_STATUS_OK;
      break;
    }
    if (status == TIDE_STATUS_OK) {
      status = tide_mux_write_packet(mux, &packet);
    }
    tide_packet_ref_reset(&packet);
    if (status != TIDE_STATUS_OK) {
      goto cleanup;
    }
  }

cleanup:
  if (mux != NULL) {
    if (status == TIDE_STATUS_OK) {
      tide_status close_status = tide_mux_close(mux);
      status = close_status;
    } else {
      tide_mux_abort(mux);
    }
  }
  tide_demux_close(demux);
  tide_source_destroy(prefix_source);
  return status;
}

uint64_t tide_repair_plan_valid_prefix(const tide_repair_plan *opaque) {
  const struct tide_repair_plan *plan = (const struct tide_repair_plan *)opaque;
  return plan == NULL ? 0u : plan->valid_prefix;
}

const tide_error *tide_repair_plan_error(const tide_repair_plan *opaque) {
  const struct tide_repair_plan *plan = (const struct tide_repair_plan *)opaque;
  return plan == NULL ? NULL : &plan->error;
}

void tide_repair_plan_destroy(tide_repair_plan *opaque) {
  free(opaque);
}
