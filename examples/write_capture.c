#include "tide/mux.h"
#include "tide/source.h"

#include <string.h>

int main(void) {
  tide_mux *mux = NULL;
  tide_stream_info stream;
  tide_packet_ref packet;
  uint8_t payload[4] = {1u, 2u, 3u, 4u};

  if (tide_mux_create(&mux, "example.tide", NULL) != TIDE_STATUS_OK) {
    return 1;
  }
  memset(&stream, 0, sizeof(stream));
  stream.track_id = 1u;
  stream.generation = 1u;
  stream.media_kind = 1u;
  stream.time_base = (tide_rational){1u, 48000u};
  if (tide_mux_add_stream(mux, &stream) != TIDE_STATUS_OK) {
    tide_mux_abort(mux);
    return 1;
  }
  tide_packet_ref_init(&packet);
  packet.info.track_id = 1u;
  packet.info.generation = 1u;
  packet.info.packet_seq = 1u;
  packet.info.duration = 1024;
  packet.lease = tide_payload_lease_create_copy(payload, sizeof(payload));
  packet.info.payload.lease = packet.lease;
  packet.info.payload.data = tide_payload_lease_data(packet.lease);
  packet.info.payload.size = tide_payload_lease_size(packet.lease);
  if (tide_mux_write_packet(mux, &packet) != TIDE_STATUS_OK) {
    tide_packet_ref_reset(&packet);
    tide_mux_abort(mux);
    return 1;
  }
  return tide_mux_close(mux) == TIDE_STATUS_OK ? 0 : 1;
}
