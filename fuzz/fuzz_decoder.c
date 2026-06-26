#include "tide/decoder.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int run_one(const uint8_t *data, size_t size) {
  tide_decoder decoder;
  size_t consumed = 0;
  tide_status status;
  if (tide_decoder_init(&decoder, NULL, NULL, NULL) != TIDE_STATUS_OK) {
    return 0;
  }
  status = tide_decoder_feed(&decoder, data, size, 1, &consumed);
  if (consumed != size) {
    abort();
  }
  if (status == TIDE_STATUS_OK) {
    uint64_t prefix = tide_decoder_valid_prefix(&decoder);
    if (prefix != (uint64_t)size) {
      abort();
    }
  }
  tide_decoder_destroy(&decoder);
  return 0;
}

#ifdef TIDE_LIBFUZZER
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  return run_one(data, size);
}
#else
int main(int argc, char **argv) {
  FILE *file;
  uint8_t *data;
  long length;
  if (argc != 2) {
    return 0;
  }
  file = fopen(argv[1], "rb");
  if (file == NULL) {
    return 1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return 1;
  }
  length = ftell(file);
  if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return 1;
  }
  data = (uint8_t *)malloc((size_t)length);
  if (data == NULL && length != 0) {
    fclose(file);
    return 1;
  }
  if (fread(data, 1u, (size_t)length, file) != (size_t)length) {
    free(data);
    fclose(file);
    return 1;
  }
  fclose(file);
  (void)run_one(data, (size_t)length);
  free(data);
  return 0;
}
#endif
