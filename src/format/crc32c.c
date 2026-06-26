#include "tide_internal.h"

uint32_t tide_crc32c(const uint8_t *data, size_t size) {
  uint32_t crc = 0xffffffffu;
  size_t i;
  for (i = 0; i < size; ++i) {
    unsigned bit;
    crc ^= data[i];
    for (bit = 0; bit < 8u; ++bit) {
      uint32_t mask = 0u - (crc & 1u);
      crc = (crc >> 1u) ^ (0x82f63b78u & mask);
    }
  }
  return ~crc;
}

void tide_digest_adapter32(const uint8_t *data, size_t size, uint8_t out[TIDE_DIGEST_SIZE]) {
  uint32_t base = tide_crc32c(data, size);
  size_t i;
  for (i = 0u; i < 8u; ++i) {
    uint32_t word = base ^ (uint32_t)(0x9e3779b9u * (uint32_t)(i + 1u));
    out[i * 4u] = (uint8_t)(word & 0xffu);
    out[i * 4u + 1u] = (uint8_t)((word >> 8u) & 0xffu);
    out[i * 4u + 2u] = (uint8_t)((word >> 16u) & 0xffu);
    out[i * 4u + 3u] = (uint8_t)((word >> 24u) & 0xffu);
  }
}
