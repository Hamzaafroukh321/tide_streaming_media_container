#include "tide_internal.h"

#include <stdlib.h>
#include <string.h>

int tide_checked_add_u64(uint64_t a, uint64_t b, uint64_t *out) {
  if (UINT64_MAX - a < b) {
    return 0;
  }
  *out = a + b;
  return 1;
}

int tide_checked_mul_u64(uint64_t a, uint64_t b, uint64_t *out) {
  if (a != 0u && b > UINT64_MAX / a) {
    return 0;
  }
  *out = a * b;
  return 1;
}

int tide_checked_u64_to_size(uint64_t value, size_t *out) {
  if (value > (uint64_t)SIZE_MAX) {
    return 0;
  }
  *out = (size_t)value;
  return 1;
}

int tide_align_u64(uint64_t value, uint64_t alignment, uint64_t *out) {
  uint64_t mask;
  uint64_t added;
  if (alignment == 0u || (alignment & (alignment - 1u)) != 0u) {
    return 0;
  }
  mask = alignment - 1u;
  if (!tide_checked_add_u64(value, mask, &added)) {
    return 0;
  }
  *out = added & ~mask;
  return 1;
}

uint64_t tide_gcd_u64(uint64_t a, uint64_t b) {
  while (b != 0u) {
    uint64_t r = a % b;
    a = b;
    b = r;
  }
  return a;
}

tide_status tide_buffer_reserve(tide_buffer *buffer, size_t additional) {
  size_t needed;
  size_t next;
  uint8_t *grown;

  if (buffer == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (additional > SIZE_MAX - buffer->size) {
    return TIDE_STATUS_RESOURCE;
  }
  needed = buffer->size + additional;
  if (needed > buffer->aggregate_limit && buffer->aggregate_limit != 0u) {
    return TIDE_STATUS_RESOURCE;
  }
  if (needed <= buffer->capacity) {
    return TIDE_STATUS_OK;
  }

  next = buffer->capacity == 0u ? 256u : buffer->capacity;
  while (next < needed) {
    if (next > SIZE_MAX / 2u) {
      next = needed;
      break;
    }
    next *= 2u;
  }
  grown = (uint8_t *)realloc(buffer->data, next);
  if (grown == NULL) {
    return TIDE_STATUS_RESOURCE;
  }
  buffer->data = grown;
  buffer->capacity = next;
  return TIDE_STATUS_OK;
}

tide_status tide_buffer_append(tide_buffer *buffer, const void *data, size_t size) {
  tide_status status;
  if (size != 0u && data == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  status = tide_buffer_reserve(buffer, size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  if (size != 0u) {
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
  }
  return TIDE_STATUS_OK;
}

tide_status tide_buffer_append_zero(tide_buffer *buffer, size_t size) {
  tide_status status = tide_buffer_reserve(buffer, size);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  if (size != 0u) {
    memset(buffer->data + buffer->size, 0, size);
    buffer->size += size;
  }
  return TIDE_STATUS_OK;
}

void tide_buffer_destroy(tide_buffer *buffer) {
  if (buffer != NULL) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
  }
}
