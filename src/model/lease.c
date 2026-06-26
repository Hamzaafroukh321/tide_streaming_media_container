#include "tide/source.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct tide_payload_lease {
  atomic_uint refcount;
  uint8_t *data;
  size_t size;
  uint64_t lease_id;
};

tide_payload_lease *tide_payload_lease_create_copy(const uint8_t *data,
                                                   size_t size) {
  static atomic_ullong next_id = 1u;
  tide_payload_lease *lease;
  if (data == NULL && size != 0u) {
    return NULL;
  }
  lease = (tide_payload_lease *)calloc(1u, sizeof(*lease));
  if (lease == NULL) {
    return NULL;
  }
  if (size != 0u) {
    lease->data = (uint8_t *)malloc(size);
    if (lease->data == NULL) {
      free(lease);
      return NULL;
    }
    memcpy(lease->data, data, size);
  }
  atomic_init(&lease->refcount, 1u);
  lease->size = size;
  lease->lease_id = (uint64_t)atomic_fetch_add(&next_id, 1u);
  return lease;
}

tide_payload_lease *tide_payload_lease_retain(tide_payload_lease *lease) {
  if (lease != NULL) {
    (void)atomic_fetch_add_explicit(&lease->refcount, 1u, memory_order_relaxed);
  }
  return lease;
}

void tide_payload_lease_release(tide_payload_lease *lease) {
  if (lease != NULL &&
      atomic_fetch_sub_explicit(&lease->refcount, 1u, memory_order_acq_rel) == 1u) {
    free(lease->data);
    lease->data = NULL;
    lease->size = 0;
    free(lease);
  }
}

const uint8_t *tide_payload_lease_data(const tide_payload_lease *lease) {
  return lease == NULL ? NULL : lease->data;
}

size_t tide_payload_lease_size(const tide_payload_lease *lease) {
  return lease == NULL ? 0u : lease->size;
}
