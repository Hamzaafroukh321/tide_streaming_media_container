#include "tide/decoder.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct event_count {
  int records;
  int streams;
  int packets;
} event_count;

static tide_status on_record(void *user, const tide_record_event *event) {
  event_count *count = (event_count *)user;
  (void)event;
  count->records += 1;
  return TIDE_STATUS_OK;
}

static tide_status on_stream(void *user, const tide_stream_info *stream) {
  event_count *count = (event_count *)user;
  (void)stream;
  count->streams += 1;
  return TIDE_STATUS_OK;
}

static tide_status on_packet(void *user, const tide_packet_info *packet) {
  event_count *count = (event_count *)user;
  (void)packet;
  count->packets += 1;
  return TIDE_STATUS_OK;
}

static tide_status decode_once(const uint8_t *data, size_t size, event_count *events) {
  tide_callbacks callbacks;
  tide_decoder decoder;
  size_t consumed = 0;
  tide_status status;
  callbacks.size = sizeof(callbacks);
  callbacks.on_record = on_record;
  callbacks.on_stream = on_stream;
  callbacks.on_packet = on_packet;
  callbacks.on_error = NULL;
  status = tide_decoder_init(&decoder, NULL, &callbacks, events);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  status = tide_decoder_feed(&decoder, data, size, 1, &consumed);
  if (consumed != size) {
    abort();
  }
  tide_decoder_destroy(&decoder);
  return status;
}

static tide_status decode_split(const uint8_t *data,
                                size_t size,
                                size_t split,
                                event_count *events) {
  tide_callbacks callbacks;
  tide_decoder decoder;
  size_t consumed = 0;
  tide_status status;
  callbacks.size = sizeof(callbacks);
  callbacks.on_record = on_record;
  callbacks.on_stream = on_stream;
  callbacks.on_packet = on_packet;
  callbacks.on_error = NULL;
  status = tide_decoder_init(&decoder, NULL, &callbacks, events);
  if (status != TIDE_STATUS_OK) {
    return status;
  }
  status = tide_decoder_feed(&decoder, data, split, 0, &consumed);
  if (status != TIDE_STATUS_OK || consumed != split) {
    tide_decoder_destroy(&decoder);
    return status;
  }
  status = tide_decoder_feed(&decoder, data + split, size - split, 1, &consumed);
  if (consumed != size - split) {
    abort();
  }
  tide_decoder_destroy(&decoder);
  return status;
}

static int run_one(const uint8_t *data, size_t size) {
  event_count contiguous = {0, 0, 0};
  event_count chunked = {0, 0, 0};
  tide_status a;
  tide_status b;
  if (size < 2u) {
    return 0;
  }
  a = decode_once(data + 1u, size - 1u, &contiguous);
  b = decode_split(data + 1u, size - 1u, (size_t)data[0] % size, &chunked);
  if (a != b ||
      contiguous.records != chunked.records ||
      contiguous.streams != chunked.streams ||
      contiguous.packets != chunked.packets) {
    abort();
  }
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
