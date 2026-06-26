#include "tide_internal.h"

#include <string.h>

void tide_reader_init(tide_reader *reader, const uint8_t *data, size_t size) {
  reader->data = data;
  reader->size = size;
  reader->offset = 0;
}

static int tide_reader_has(const tide_reader *reader, size_t size) {
  return reader->offset <= reader->size && size <= reader->size - reader->offset;
}

int tide_reader_read_u8(tide_reader *reader, uint8_t *out) {
  if (!tide_reader_has(reader, 1u)) {
    return 0;
  }
  *out = reader->data[reader->offset];
  reader->offset += 1u;
  return 1;
}

int tide_reader_read_u16(tide_reader *reader, uint16_t *out) {
  const uint8_t *p;
  if (!tide_reader_has(reader, 2u)) {
    return 0;
  }
  p = reader->data + reader->offset;
  *out = (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
  reader->offset += 2u;
  return 1;
}

int tide_reader_read_u32(tide_reader *reader, uint32_t *out) {
  const uint8_t *p;
  if (!tide_reader_has(reader, 4u)) {
    return 0;
  }
  p = reader->data + reader->offset;
  *out = (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
         ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
  reader->offset += 4u;
  return 1;
}

int tide_reader_read_u64(tide_reader *reader, uint64_t *out) {
  uint32_t lo;
  uint32_t hi;
  if (!tide_reader_read_u32(reader, &lo) || !tide_reader_read_u32(reader, &hi)) {
    return 0;
  }
  *out = (uint64_t)lo | ((uint64_t)hi << 32u);
  return 1;
}

int tide_reader_read_i64(tide_reader *reader, int64_t *out) {
  uint64_t raw;
  if (!tide_reader_read_u64(reader, &raw)) {
    return 0;
  }
  memcpy(out, &raw, sizeof(raw));
  return 1;
}

int tide_reader_read_bytes(tide_reader *reader, const uint8_t **out, size_t size) {
  if (!tide_reader_has(reader, size)) {
    return 0;
  }
  *out = reader->data + reader->offset;
  reader->offset += size;
  return 1;
}

int tide_reader_skip(tide_reader *reader, size_t size) {
  const uint8_t *unused;
  return tide_reader_read_bytes(reader, &unused, size);
}

int tide_reader_read_uleb128(tide_reader *reader, uint64_t *out) {
  uint64_t value = 0;
  unsigned shift = 0;
  unsigned i;
  for (i = 0; i < TIDE_MAX_HEADER_VARINT_BYTES; ++i) {
    uint8_t byte;
    if (!tide_reader_read_u8(reader, &byte)) {
      return 0;
    }
    if (shift == 63u && (byte & 0x7eu) != 0u) {
      return 0;
    }
    value |= ((uint64_t)(byte & 0x7fu)) << shift;
    if ((byte & 0x80u) == 0u) {
      if (i > 0u && byte == 0u) {
        return 0;
      }
      *out = value;
      return 1;
    }
    shift += 7u;
  }
  return 0;
}

void tide_write_u16(uint8_t *dst, uint16_t value) {
  dst[0] = (uint8_t)(value & 0xffu);
  dst[1] = (uint8_t)((value >> 8u) & 0xffu);
}

void tide_write_u32(uint8_t *dst, uint32_t value) {
  dst[0] = (uint8_t)(value & 0xffu);
  dst[1] = (uint8_t)((value >> 8u) & 0xffu);
  dst[2] = (uint8_t)((value >> 16u) & 0xffu);
  dst[3] = (uint8_t)((value >> 24u) & 0xffu);
}

void tide_write_u64(uint8_t *dst, uint64_t value) {
  tide_write_u32(dst, (uint32_t)(value & 0xffffffffu));
  tide_write_u32(dst + 4, (uint32_t)(value >> 32u));
}

void tide_write_i64(uint8_t *dst, int64_t value) {
  uint64_t raw;
  memcpy(&raw, &value, sizeof(raw));
  tide_write_u64(dst, raw);
}

size_t tide_uleb128_size(uint64_t value) {
  size_t size = 1u;
  while (value >= 0x80u) {
    value >>= 7u;
    ++size;
  }
  return size;
}

size_t tide_write_uleb128(uint8_t *dst, uint64_t value) {
  size_t count = 0;
  do {
    uint8_t byte = (uint8_t)(value & 0x7fu);
    value >>= 7u;
    if (value != 0u) {
      byte |= 0x80u;
    }
    dst[count] = byte;
    ++count;
  } while (value != 0u);
  return count;
}
