#ifndef TIDE_DEMUX_H
#define TIDE_DEMUX_H

#include "tide/source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tide_demux_options {
  size_t size;
  tide_limits limits;
  uint32_t selected_track;
  int strict;
} tide_demux_options;

tide_status tide_demux_open(tide_demux **out,
                            tide_source *source,
                            const tide_demux_options *options);
tide_status tide_demux_next(tide_demux *demux, tide_packet_ref *out);
size_t tide_demux_stream_count(const tide_demux *demux);
tide_status tide_demux_stream_info(const tide_demux *demux,
                                   size_t index,
                                   tide_stream_info *out);
const tide_error *tide_demux_last_error(const tide_demux *demux);
void tide_demux_close(tide_demux *demux);

#ifdef __cplusplus
}
#endif

#endif
