#include "tide_internal.h"

#include <stdlib.h>

static int tide_packet_less(const tide_packet_ref *a,
                            uint64_t epoch_a,
                            const tide_packet_ref *b,
                            uint64_t epoch_b) {
  if (epoch_a != epoch_b) {
    return epoch_a < epoch_b;
  }
  if (a->info.pts != b->info.pts) {
    return a->info.pts < b->info.pts;
  }
  if (a->info.dts != b->info.dts) {
    return a->info.dts < b->info.dts;
  }
  return a->info.packet_seq < b->info.packet_seq;
}

tide_status tide_reorder_queue_init(tide_reorder_queue *queue, size_t limit) {
  if (queue == NULL || limit == 0u) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  queue->items = NULL;
  queue->count = 0;
  queue->capacity = 0;
  queue->limit = limit;
  return TIDE_STATUS_OK;
}

void tide_reorder_queue_destroy(tide_reorder_queue *queue) {
  if (queue != NULL) {
    size_t i;
    for (i = 0; i < queue->count; ++i) {
      tide_packet_ref_reset(&queue->items[i].packet);
    }
    free(queue->items);
    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
  }
}

tide_status tide_reorder_queue_push(tide_reorder_queue *queue,
                                    tide_packet_ref *packet,
                                    uint64_t epoch) {
  size_t pos;
  if (queue == NULL || packet == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (queue->count >= queue->limit) {
    return TIDE_STATUS_RESOURCE;
  }
  if (queue->count == queue->capacity) {
    size_t next = queue->capacity == 0u ? 8u : queue->capacity * 2u;
    tide_reorder_item *grown;
    if (next > queue->limit) {
      next = queue->limit;
    }
    grown = (tide_reorder_item *)realloc(queue->items, next * sizeof(*grown));
    if (grown == NULL) {
      return TIDE_STATUS_RESOURCE;
    }
    queue->items = grown;
    queue->capacity = next;
  }
  pos = queue->count;
  while (pos > 0u &&
         tide_packet_less(packet, epoch, &queue->items[pos - 1u].packet, queue->items[pos - 1u].epoch)) {
    queue->items[pos] = queue->items[pos - 1u];
    --pos;
  }
  tide_packet_ref_init(&queue->items[pos].packet);
  queue->items[pos].epoch = epoch;
  queue->count += 1u;
  return tide_packet_ref_move(&queue->items[pos].packet, packet);
}

tide_status tide_reorder_queue_pop_ready(tide_reorder_queue *queue,
                                         tide_packet_ref *out) {
  size_t i;
  if (queue == NULL || out == NULL) {
    return TIDE_STATUS_INVALID_ARGUMENT;
  }
  if (queue->count == 0u) {
    return TIDE_STATUS_WOULD_BLOCK;
  }
  tide_packet_ref_init(out);
  (void)tide_packet_ref_move(out, &queue->items[0].packet);
  for (i = 1u; i < queue->count; ++i) {
    queue->items[i - 1u] = queue->items[i];
  }
  queue->count -= 1u;
  return TIDE_STATUS_OK;
}
