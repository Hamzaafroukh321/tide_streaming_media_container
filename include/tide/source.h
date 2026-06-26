#ifndef TIDE_SOURCE_H
#define TIDE_SOURCE_H

#include <stdio.h>

#include "tide/tide.h"

#ifdef __cplusplus
extern "C" {
#endif

tide_status tide_source_from_memory(tide_source **out,
                                    const uint8_t *data,
                                    size_t size);
tide_status tide_source_from_file(tide_source **out, const char *path);
void tide_source_destroy(tide_source *source);
const uint8_t *tide_source_data(const tide_source *source);
size_t tide_source_size(const tide_source *source);

tide_payload_lease *tide_payload_lease_create_copy(const uint8_t *data,
                                                   size_t size);
tide_payload_lease *tide_payload_lease_retain(tide_payload_lease *lease);
void tide_payload_lease_release(tide_payload_lease *lease);
const uint8_t *tide_payload_lease_data(const tide_payload_lease *lease);
size_t tide_payload_lease_size(const tide_payload_lease *lease);

#ifdef __cplusplus
}
#endif

#endif
