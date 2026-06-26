#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

tide_status tide_source_from_memory(tide_source **out,
                                    const uint8_t *data,
                                    size_t size) {
  tide_source *source;
  if (out == NULL || (data == NULL && size != 0u)) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = NULL;
  source = (tide_source *)calloc(1u, sizeof(*source));
  if (source == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  if (size != 0u) {
    source->data = (uint8_t *)malloc(size);
    if (source->data == NULL) {
      free(source);
      return TIDE_STATUS_RESOURCE;
    }
    memcpy(source->data, data, size);
  }
  source->size = size;
  source->generation = 1u;
  *out = source;
  return TIDE_STATUS_OK;
}

tide_status tide_source_from_file(tide_source **out, const char *path) {
  FILE *file = NULL;
  long length;
  tide_source *source = NULL;
  tide_status status = TIDE_STATUS_OK;

  if (out == NULL || path == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  *out = NULL;
  file = fopen(path, "rb");
  if (file == NULL) {
    return TIDE_STATUS_IO;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    status = TIDE_STATUS_IO;
    goto cleanup;
  }
  length = ftell(file);
  if (length < 0) {
    status = TIDE_STATUS_IO;
    goto cleanup;
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    status = TIDE_STATUS_IO;
    goto cleanup;
  }
  source = (tide_source *)calloc(1u, sizeof(*source));
  if (source == NULL) {
    status = TIDE_STATUS_RESOURCE;
    goto cleanup;
  }
  if (length != 0) {
    source->data = (uint8_t *)malloc((size_t)length);
    if (source->data == NULL) {
      status = TIDE_STATUS_RESOURCE;
      goto cleanup;
    }
    if (fread(source->data, 1u, (size_t)length, file) != (size_t)length) {
      status = TIDE_STATUS_IO;
      goto cleanup;
    }
  }
  source->size = (size_t)length;
  source->generation = 1u;
  *out = source;
  source = NULL;

cleanup:
  if (file != NULL) {
    fclose(file);
  }
  tide_source_destroy(source);
  return status;
}
