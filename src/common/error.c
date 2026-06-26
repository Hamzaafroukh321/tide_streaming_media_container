#include "tide/error.h"

#include <string.h>

const char *tide_status_string(tide_status status) {
  switch (status) {
    case TIDE_STATUS_OK: return "OK";
    case TIDE_STATUS_PARTIAL: return "PARTIAL";
    case TIDE_STATUS_WOULD_BLOCK: return "WOULD_BLOCK";
    case TIDE_STATUS_CANCELLED: return "CANCELLED";
    case TIDE_STATUS_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case TIDE_STATUS_FORMAT: return "FORMAT";
    case TIDE_STATUS_TRUNCATED: return "TRUNCATED";
    case TIDE_STATUS_UNSUPPORTED: return "UNSUPPORTED";
    case TIDE_STATUS_INTEGRITY: return "INTEGRITY";
    case TIDE_STATUS_TIMELINE: return "TIMELINE";
    case TIDE_STATUS_INDEX: return "INDEX";
    case TIDE_STATUS_RESOURCE: return "RESOURCE";
    case TIDE_STATUS_IO: return "IO";
    case TIDE_STATUS_INTERNAL: return "INTERNAL";
    default: return "UNKNOWN";
  }
}

void tide_error_clear(tide_error *error) {
  if (error != NULL) {
    memset(error, 0, sizeof(*error));
  }
}

static void tide_copy_text(char *dst, size_t dst_size, const char *src) {
  size_t i = 0;
  if (dst_size == 0) {
    return;
  }
  if (src != NULL) {
    while (src[i] != '\0' && i + 1u < dst_size) {
      dst[i] = src[i];
      ++i;
    }
  }
  dst[i] = '\0';
}

void tide_error_set(tide_error *error,
                    tide_status code,
                    tide_severity severity,
                    uint64_t offset,
                    uint16_t record_type,
                    uint8_t depth,
                    const char *component,
                    const char *message) {
  if (error == NULL) {
    return;
  }
  tide_error_clear(error);
  error->code = code;
  error->severity = severity;
  error->offset = offset;
  error->record_type = record_type;
  error->depth = depth;
  tide_copy_text(error->component, sizeof(error->component), component);
  tide_copy_text(error->message, sizeof(error->message), message);
}
