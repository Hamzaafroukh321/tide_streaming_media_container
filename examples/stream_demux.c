#include "tide/demux.h"

#include <stdio.h>

int main(int argc, char **argv) {
  tide_source *source = NULL;
  tide_demux *demux = NULL;
  tide_packet_ref packet;
  tide_status status;

  if (argc != 2) {
    return 2;
  }
  status = tide_source_from_file(&source, argv[1]);
  if (status == TIDE_STATUS_OK) {
    status = tide_demux_open(&demux, source, NULL);
  }
  if (status != TIDE_STATUS_OK) {
    fprintf(stderr, "%s\n", tide_status_string(status));
    tide_source_destroy(source);
    return 1;
  }
  tide_packet_ref_init(&packet);
  while (tide_demux_next(demux, &packet) == TIDE_STATUS_OK) {
    printf("track=%u seq=%llu payload=%zu\n",
           packet.info.track_id,
           (unsigned long long)packet.info.packet_seq,
           packet.info.payload.size);
    tide_packet_ref_reset(&packet);
  }
  tide_demux_close(demux);
  tide_source_destroy(source);
  return 0;
}
