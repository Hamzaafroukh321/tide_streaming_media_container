#include "tide_internal.h"

#include <stdlib.h>

void tide_source_destroy(tide_source *source) {
  if (source != NULL) {
    free(source->data);
    source->data = NULL;
    source->size = 0;
    free(source);
  }
}

const uint8_t *tide_source_data(const tide_source *source) {
  return source == NULL ? NULL : source->data;
}

size_t tide_source_size(const tide_source *source) {
  return source == NULL ? 0u : source->size;
}
