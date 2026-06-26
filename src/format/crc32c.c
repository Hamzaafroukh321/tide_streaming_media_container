#include "tide/tide.h"

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
