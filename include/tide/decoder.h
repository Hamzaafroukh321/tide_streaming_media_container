#ifndef TIDE_DECODER_H
#define TIDE_DECODER_H

#include "tide/tide.h"

#ifdef __cplusplus
extern "C" {
#endif

tide_status tide_decoder_init(tide_decoder *decoder,
                              const tide_limits *limits,
                              const tide_callbacks *callbacks,
                              void *user);
tide_status tide_decoder_feed(tide_decoder *decoder,
                              const uint8_t *data,
                              size_t size,
                              int end,
                              size_t *consumed);
void tide_decoder_cancel(tide_decoder *decoder);
void tide_decoder_destroy(tide_decoder *decoder);
const tide_error *tide_decoder_last_error(const tide_decoder *decoder);
uint64_t tide_decoder_valid_prefix(const tide_decoder *decoder);

#ifdef __cplusplus
}
#endif

#endif
