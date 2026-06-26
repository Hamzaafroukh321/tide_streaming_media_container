#ifndef TIDE_INTEGRATION_TEST_FIXTURE_H
#define TIDE_INTEGRATION_TEST_FIXTURE_H

#include "test_harness.h"

#include "tide/mux.h"
#include "tide/source.h"

#include <stdio.h>
#include <string.h>

static int tide_write_integration_sample(const char *path) {
  tide_mux *mux = NULL;
  tide_mux_options options;
  tide_stream_info stream;
  tide_packet_ref packet;
  uint8_t first[3] = {9u, 8u, 7u};
  uint8_t second[2] = {6u, 5u};
  memset(&options, 0, sizeof(options));
  options.size = sizeof(options);
  options.movie_time_base = (tide_rational){1u, 1000u};
  options.write_footer = 1;
  TIDE_EXPECT_STATUS(tide_mux_create(&mux, path, &options), TIDE_STATUS_OK);
  memset(&stream, 0, sizeof(stream));
  stream.track_id = 7u;
  stream.generation = 1u;
  stream.media_kind = 1u;
  stream.codec_tag = 0x54455354u;
  stream.time_base = (tide_rational){1u, 90000u};
  TIDE_EXPECT_STATUS(tide_mux_add_stream(mux, &stream), TIDE_STATUS_OK);

  tide_packet_ref_init(&packet);
  packet.info.track_id = 7u;
  packet.info.generation = 1u;
  packet.info.packet_seq = 1u;
  packet.info.duration = 3000;
  packet.info.payload.data = first;
  packet.info.payload.size = sizeof(first);
  packet.lease = tide_payload_lease_create_copy(first, sizeof(first));
  packet.info.payload.lease = packet.lease;
  packet.info.payload.data = tide_payload_lease_data(packet.lease);
  TIDE_EXPECT_STATUS(tide_mux_write_packet(mux, &packet), TIDE_STATUS_OK);

  tide_packet_ref_init(&packet);
  packet.info.track_id = 7u;
  packet.info.generation = 1u;
  packet.info.packet_seq = 2u;
  packet.info.dts = 3000;
  packet.info.pts = 3000;
  packet.info.duration = 3000;
  packet.info.payload.data = second;
  packet.info.payload.size = sizeof(second);
  packet.lease = tide_payload_lease_create_copy(second, sizeof(second));
  packet.info.payload.lease = packet.lease;
  packet.info.payload.data = tide_payload_lease_data(packet.lease);
  TIDE_EXPECT_STATUS(tide_mux_write_packet(mux, &packet), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_mux_close(mux), TIDE_STATUS_OK);
  return 0;
}

#endif
