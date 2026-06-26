#ifndef TIDE_REPAIR_H
#define TIDE_REPAIR_H

#include "tide/source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tide_repair_options {
  size_t size;
  tide_limits limits;
  int require_checkpoint;
} tide_repair_options;

tide_status tide_repair_scan(tide_source *source,
                             const tide_repair_options *options,
                             tide_repair_plan **out);
tide_status tide_repair_write(tide_repair_plan *plan, const char *path);
uint64_t tide_repair_plan_valid_prefix(const tide_repair_plan *plan);
const tide_error *tide_repair_plan_error(const tide_repair_plan *plan);
void tide_repair_plan_destroy(tide_repair_plan *plan);

#ifdef __cplusplus
}
#endif

#endif
