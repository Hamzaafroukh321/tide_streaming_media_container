#include "tide/demux.h"

#include <stdio.h>

int main(int argc, char **argv) {
  tide_source *source = NULL;
  tide_demux *demux = NULL;
  tide_status status;
  if (argc != 2) {
    return 2;
  }
  status = tide_source_from_file(&source, argv[1]);
  if (status == TIDE_STATUS_OK) {
    status = tide_demux_open(&demux, source, NULL);
  }
  printf("index_status=%s streams=%zu\n",
         tide_status_string(status),
         tide_demux_stream_count(demux));
  tide_demux_close(demux);
  tide_source_destroy(source);
  return status == TIDE_STATUS_OK ? 0 : 1;
}
