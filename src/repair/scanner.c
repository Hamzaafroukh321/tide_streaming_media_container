#include "tide_internal.h"

#include <stdlib.h>

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
  FILE *file;
  if (plan == NULL || path == NULL || plan->valid_prefix == 0u) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  file = fopen(path, "wb");
  if (file == NULL) {
    return TIDE_STATUS_IO;
  }
  if (fwrite(tide_source_data(plan->source), 1u, (size_t)plan->valid_prefix, file) != (size_t)plan->valid_prefix) {
    fclose(file);
    return TIDE_STATUS_IO;
  }
  if (fclose(file) != 0) {
    return TIDE_STATUS_IO;
  }
  return TIDE_STATUS_OK;
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
