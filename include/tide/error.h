#ifndef TIDE_ERROR_H
#define TIDE_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tide_status {
  TIDE_STATUS_OK = 0,
  TIDE_STATUS_PARTIAL = 1,
  TIDE_STATUS_WOULD_BLOCK = 2,
  TIDE_STATUS_CANCELLED = 3,
  TIDE_STATUS_INVALID_ARGUMENT = 4,
  TIDE_STATUS_FORMAT = 5,
  TIDE_STATUS_TRUNCATED = 6,
  TIDE_STATUS_UNSUPPORTED = 7,
  TIDE_STATUS_INTEGRITY = 8,
  TIDE_STATUS_TIMELINE = 9,
  TIDE_STATUS_INDEX = 10,
  TIDE_STATUS_RESOURCE = 11,
  TIDE_STATUS_IO = 12,
  TIDE_STATUS_INTERNAL = 13
} tide_status;

typedef enum tide_severity {
  TIDE_SEVERITY_INFO = 0,
  TIDE_SEVERITY_WARNING = 1,
  TIDE_SEVERITY_ERROR = 2,
  TIDE_SEVERITY_FATAL = 3
} tide_severity;

typedef struct tide_error {
  tide_status code;
  tide_severity severity;
  uint64_t offset;
  uint16_t record_type;
  uint8_t depth;
  uint32_t track_id;
  uint32_t generation;
  uint64_t packet_seq;
  char component[24];
  char message[120];
} tide_error;

const char *tide_status_string(tide_status status);
void tide_error_clear(tide_error *error);
void tide_error_set(tide_error *error,
                    tide_status code,
                    tide_severity severity,
                    uint64_t offset,
                    uint16_t record_type,
                    uint8_t depth,
                    const char *component,
                    const char *message);

#ifdef __cplusplus
}
#endif

#endif
