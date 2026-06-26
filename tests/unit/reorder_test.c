#include "test_harness.h"

#include "tide_internal.h"

static int reorder_depth_release(void) {
  tide_reorder_queue queue;
  tide_packet_ref a;
  tide_packet_ref b;
  tide_packet_ref out;
  uint8_t payload_a[1] = {1u};
  uint8_t payload_b[1] = {2u};
  TIDE_EXPECT_STATUS(tide_reorder_queue_init(&queue, 4u), TIDE_STATUS_OK);
  tide_packet_ref_init(&a);
  tide_packet_ref_init(&b);
  a.info.pts = 20;
  a.info.dts = 0;
  a.info.packet_seq = 2;
  a.lease = tide_payload_lease_create_copy(payload_a, sizeof(payload_a));
  a.info.payload.data = tide_payload_lease_data(a.lease);
  a.info.payload.size = tide_payload_lease_size(a.lease);
  b.info.pts = 10;
  b.info.dts = 0;
  b.info.packet_seq = 1;
  b.lease = tide_payload_lease_create_copy(payload_b, sizeof(payload_b));
  b.info.payload.data = tide_payload_lease_data(b.lease);
  b.info.payload.size = tide_payload_lease_size(b.lease);
  TIDE_EXPECT_STATUS(tide_reorder_queue_push(&queue, &a, 0u), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_reorder_queue_push(&queue, &b, 0u), TIDE_STATUS_OK);
  tide_packet_ref_init(&out);
  TIDE_EXPECT_STATUS(tide_reorder_queue_pop_ready(&queue, &out), TIDE_STATUS_OK);
  TIDE_EXPECT(out.info.packet_seq == 1u);
  tide_packet_ref_reset(&out);
  TIDE_EXPECT_STATUS(tide_reorder_queue_pop_ready(&queue, &out), TIDE_STATUS_OK);
  TIDE_EXPECT(out.info.packet_seq == 2u);
  tide_packet_ref_reset(&out);
  tide_reorder_queue_destroy(&queue);
  return 0;
}

int tide_reorder_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"ReorderDepthRelease", reorder_depth_release};
  return n;
}
