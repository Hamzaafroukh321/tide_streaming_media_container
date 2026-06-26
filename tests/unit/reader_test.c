#include "test_harness.h"

#include "tide/decoder.h"
#include "tide/mux.h"
#include "tide/source.h"

#include <stdio.h>
#include <string.h>

static int write_sample(const char *path) {
  tide_mux *mux = NULL;
  tide_mux_options options;
  tide_stream_info stream;
  tide_packet_ref packet;
  uint8_t payload[4] = {1u, 2u, 3u, 4u};
  memset(&options, 0, sizeof(options));
  options.size = sizeof(options);
  options.movie_time_base = (tide_rational){1u, 1000u};
  options.write_footer = 1;
  TIDE_EXPECT_STATUS(tide_mux_create(&mux, path, &options), TIDE_STATUS_OK);
  memset(&stream, 0, sizeof(stream));
  stream.track_id = 1u;
  stream.generation = 1u;
  stream.time_base = (tide_rational){1u, 48000u};
  TIDE_EXPECT_STATUS(tide_mux_add_stream(mux, &stream), TIDE_STATUS_OK);
  tide_packet_ref_init(&packet);
  packet.info.track_id = 1u;
  packet.info.generation = 1u;
  packet.info.packet_seq = 1u;
  packet.info.duration = 1024;
  packet.lease = tide_payload_lease_create_copy(payload, sizeof(payload));
  packet.info.payload.data = tide_payload_lease_data(packet.lease);
  packet.info.payload.size = tide_payload_lease_size(packet.lease);
  TIDE_EXPECT_STATUS(tide_mux_write_packet(mux, &packet), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_mux_close(mux), TIDE_STATUS_OK);
  return 0;
}

typedef struct count_ctx {
  int records;
  int streams;
  int packets;
} count_ctx;

static tide_status on_record(void *user, const tide_record_event *event) {
  count_ctx *ctx = (count_ctx *)user;
  (void)event;
  ctx->records += 1;
  return TIDE_STATUS_OK;
}

static tide_status on_stream(void *user, const tide_stream_info *stream) {
  count_ctx *ctx = (count_ctx *)user;
  (void)stream;
  ctx->streams += 1;
  return TIDE_STATUS_OK;
}

static tide_status on_packet(void *user, const tide_packet_info *packet) {
  count_ctx *ctx = (count_ctx *)user;
  if (packet->payload.size != 4u) {
    return TIDE_STATUS_FORMAT;
  }
  ctx->packets += 1;
  return TIDE_STATUS_OK;
}

static int header_canonical_round_trip(void) {
  tide_source *source = NULL;
  tide_decoder decoder;
  tide_callbacks callbacks;
  size_t consumed = 0;
  count_ctx ctx = {0, 0, 0};
  TIDE_EXPECT(write_sample("reader_sample.tide") == 0);
  TIDE_EXPECT_STATUS(tide_source_from_file(&source, "reader_sample.tide"), TIDE_STATUS_OK);
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.size = sizeof(callbacks);
  callbacks.on_record = on_record;
  callbacks.on_stream = on_stream;
  callbacks.on_packet = on_packet;
  TIDE_EXPECT_STATUS(tide_decoder_init(&decoder, NULL, &callbacks, &ctx), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_decoder_feed(&decoder, tide_source_data(source), tide_source_size(source), 1, &consumed), TIDE_STATUS_OK);
  TIDE_EXPECT(consumed == tide_source_size(source));
  TIDE_EXPECT(ctx.records >= 3);
  TIDE_EXPECT(ctx.streams == 1);
  TIDE_EXPECT(ctx.packets == 1);
  tide_decoder_destroy(&decoder);
  tide_source_destroy(source);
  remove("reader_sample.tide");
  return 0;
}

int tide_reader_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"HeaderCanonicalRoundTrip", header_canonical_round_trip};
  return n;
}
