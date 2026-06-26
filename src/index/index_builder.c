#include "tide_internal.h"

#include <stdlib.h>

tide_status tide_index_builder_add(tide_index_entry **entries,
                                   size_t *count,
                                   size_t *capacity,
                                   const tide_index_entry *entry) {
  tide_index_entry *grown;
  size_t next;
  if (entries == NULL || count == NULL || capacity == NULL || entry == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (*count != 0u && entry->record_offset < (*entries)[*count - 1u].record_offset) {
    return TIDE_STATUS_INDEX;
  }
  if (*count == *capacity) {
    next = *capacity == 0u ? 16u : *capacity * 2u;
    grown = (tide_index_entry *)realloc(*entries, next * sizeof(*grown));
    if (grown == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    *entries = grown;
    *capacity = next;
  }
  (*entries)[*count] = *entry;
  *count += 1u;
  return TIDE_STATUS_OK;
}
