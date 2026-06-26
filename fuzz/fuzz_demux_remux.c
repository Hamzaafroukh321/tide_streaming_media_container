#include "tide/demux.h"
#include "tide/mux.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int run_one(const uint8_t *data, size_t size) {
  tide_source *source = NULL;
  tide_source *roundtrip_source = NULL;
  tide_demux *demux = NULL;
  tide_demux *roundtrip_demux = NULL;
  tide_mux *mux = NULL;
  tide_status status;
  size_t input_packets = 0u;
  size_t output_packets = 0u;
  size_t i;
  if (tide_source_from_memory(&source, data, size) != TIDE_STATUS_OK) {
    return 0;
  }
  status = tide_demux_open(&demux, source, NULL);
  if (status != TIDE_STATUS_OK) {
    tide_source_destroy(source);
    return 0;
  }
  status = tide_mux_create(&mux, "fuzz_roundtrip.tide", NULL);
  if (status == TIDE_STATUS_OK) {
    for (i = 0; i < tide_demux_stream_count(demux); ++i) {
      tide_stream_info stream;
      status = tide_demux_stream_info(demux, i, &stream);
      if (status == TIDE_STATUS_OK) {
        status = tide_mux_add_stream(mux, &stream);
      }
      if (status != TIDE_STATUS_OK) {
        break;
      }
    }
  }
  while (status == TIDE_STATUS_OK) {
    tide_packet_ref packet;
    tide_packet_ref_init(&packet);
    status = tide_demux_next(demux, &packet);
    if (status == TIDE_STATUS_WOULD_BLOCK) {
      status = TIDE_STATUS_OK;
      break;
    }
    if (status == TIDE_STATUS_OK) {
      status = tide_mux_write_packet(mux, &packet);
      input_packets += 1u;
    }
    tide_packet_ref_reset(&packet);
  }
  if (mux != NULL) {
    tide_status close_status = tide_mux_close(mux);
    if (status == TIDE_STATUS_OK) {
      status = close_status;
    }
  }
  if (status == TIDE_STATUS_OK &&
      tide_source_from_file(&roundtrip_source, "fuzz_roundtrip.tide") == TIDE_STATUS_OK &&
      tide_demux_open(&roundtrip_demux, roundtrip_source, NULL) == TIDE_STATUS_OK) {
    if (tide_demux_stream_count(roundtrip_demux) != tide_demux_stream_count(demux)) {
      abort();
    }
    for (;;) {
      tide_packet_ref packet;
      tide_packet_ref_init(&packet);
      status = tide_demux_next(roundtrip_demux, &packet);
      if (status == TIDE_STATUS_WOULD_BLOCK) {
        status = TIDE_STATUS_OK;
        break;
      }
      if (status != TIDE_STATUS_OK) {
        break;
      }
      output_packets += 1u;
      tide_packet_ref_reset(&packet);
    }
    if (status == TIDE_STATUS_OK && output_packets != input_packets) {
      abort();
    }
  }
  tide_demux_close(roundtrip_demux);
  tide_source_destroy(roundtrip_source);
  tide_demux_close(demux);
  tide_source_destroy(source);
  remove("fuzz_roundtrip.tide");
  return 0;
}

#ifdef TIDE_LIBFUZZER
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  return run_one(data, size);
}
#else
int main(void) {
  static const uint8_t empty[1] = {0u};
  return run_one(empty, sizeof(empty));
}
#endif
