#include "tide/tide.h"

#include "tide/source.h"

#include <string.h>

void tide_packet_ref_init(tide_packet_ref *packet) {
  if (packet != NULL) {
    memset(packet, 0, sizeof(*packet));
  }
}

void tide_packet_ref_reset(tide_packet_ref *packet) {
  if (packet != NULL) {
    tide_payload_lease_release(packet->lease);
    memset(packet, 0, sizeof(*packet));
  }
}

tide_status tide_packet_ref_move(tide_packet_ref *dst, tide_packet_ref *src) {
  if (dst == NULL || src == NULL || dst == src) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  tide_packet_ref_reset(dst);
  *dst = *src;
  memset(src, 0, sizeof(*src));
  if (dst->lease != NULL) {
    dst->info.payload.lease = dst->lease;
    dst->info.payload.data = tide_payload_lease_data(dst->lease);
    dst->info.payload.size = tide_payload_lease_size(dst->lease);
  }
  return TIDE_STATUS_OK;
}
