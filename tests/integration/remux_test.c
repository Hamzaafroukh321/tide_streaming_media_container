#include "test_fixture.h"

#include "tide/demux.h"

static int demux_remux_payload_identity(void) {
  tide_source *source = NULL;
  tide_demux *demux = NULL;
  tide_mux *mux = NULL;
  tide_source *remuxed = NULL;
  tide_demux *demux2 = NULL;
  tide_packet_ref a;
  tide_packet_ref b;
  tide_status status;
  size_t i;
  TIDE_EXPECT(tide_write_integration_sample("remux_in.tide") == 0);
  TIDE_EXPECT_STATUS(tide_source_from_file(&source, "remux_in.tide"), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_demux_open(&demux, source, NULL), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_mux_create(&mux, "remux_out.tide", NULL), TIDE_STATUS_OK);
  for (i = 0; i < tide_demux_stream_count(demux); ++i) {
    tide_stream_info stream;
    TIDE_EXPECT_STATUS(tide_demux_stream_info(demux, i, &stream), TIDE_STATUS_OK);
    TIDE_EXPECT_STATUS(tide_mux_add_stream(mux, &stream), TIDE_STATUS_OK);
  }
  for (;;) {
    tide_packet_ref packet;
    tide_packet_ref_init(&packet);
    status = tide_demux_next(demux, &packet);
    if (status == TIDE_STATUS_WOULD_BLOCK) {
      break;
    }
    TIDE_EXPECT_STATUS(status, TIDE_STATUS_OK);
    TIDE_EXPECT_STATUS(tide_mux_write_packet(mux, &packet), TIDE_STATUS_OK);
  }
  TIDE_EXPECT_STATUS(tide_mux_close(mux), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_source_from_file(&remuxed, "remux_out.tide"), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_demux_open(&demux2, remuxed, NULL), TIDE_STATUS_OK);
  tide_packet_ref_init(&a);
  tide_packet_ref_init(&b);
  TIDE_EXPECT_STATUS(tide_demux_next(demux2, &a), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_demux_next(demux2, &b), TIDE_STATUS_OK);
  TIDE_EXPECT(a.info.payload.size == 3u);
  TIDE_EXPECT(b.info.payload.size == 2u);
  TIDE_EXPECT(a.info.payload.data[0] == 9u);
  TIDE_EXPECT(b.info.payload.data[1] == 5u);
  tide_packet_ref_reset(&a);
  tide_packet_ref_reset(&b);
  tide_demux_close(demux2);
  tide_source_destroy(remuxed);
  tide_demux_close(demux);
  tide_source_destroy(source);
  remove("remux_in.tide");
  remove("remux_out.tide");
  return 0;
}

int tide_remux_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"DemuxRemuxPayloadIdentity", demux_remux_payload_identity};
  return n;
}
