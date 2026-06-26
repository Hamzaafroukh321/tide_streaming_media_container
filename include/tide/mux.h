#ifndef TIDE_MUX_H
#define TIDE_MUX_H

#include "tide/tide.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tide_mux_options {
  size_t size;
  tide_limits limits;
  tide_rational movie_time_base;
  int write_footer;
} tide_mux_options;

tide_status tide_mux_create(tide_mux **out,
                            const char *path,
                            const tide_mux_options *options);
tide_status tide_mux_add_stream(tide_mux *mux, const tide_stream_info *stream);
tide_status tide_mux_write_packet(tide_mux *mux, tide_packet_ref *moved_packet);
tide_status tide_mux_close(tide_mux *mux);
void tide_mux_abort(tide_mux *mux);
const tide_error *tide_mux_last_error(const tide_mux *mux);

#ifdef __cplusplus
}
#endif

#endif
