#include "test_fixture.h"

#include "tide/decoder.h"
#include "tide/source.h"

static int partial_tail_status(void) {
  tide_source *source = NULL;
  tide_decoder decoder;
  size_t consumed = 0;
  size_t truncated_size;
  TIDE_EXPECT(tide_write_integration_sample("partial_in.tide") == 0);
  TIDE_EXPECT_STATUS(tide_source_from_file(&source, "partial_in.tide"), TIDE_STATUS_OK);
  truncated_size = tide_source_size(source) - 2u;
  TIDE_EXPECT_STATUS(tide_decoder_init(&decoder, NULL, NULL, NULL), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_decoder_feed(&decoder, tide_source_data(source), truncated_size, 1, &consumed), TIDE_STATUS_PARTIAL);
  TIDE_EXPECT(tide_decoder_valid_prefix(&decoder) < tide_source_size(source));
  TIDE_EXPECT(consumed == truncated_size);
  tide_decoder_destroy(&decoder);
  tide_source_destroy(source);
  remove("partial_in.tide");
  return 0;
}

int tide_partial_file_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"PartialTailStatus", partial_tail_status};
  return n;
}
