# TIDE-1 Format

This document describes the implemented TIDE-1 subset.

## Header

The header is 72 bytes:

| Offset | Field |
|---:|---|
| 0 | 8-byte magic `54 49 44 45 31 0D 0A 1A` |
| 8 | little-endian `major:u16`, must be `1` |
| 10 | little-endian `minor:u16` |
| 12 | 16-byte nonzero file UUID |
| 28 | movie time-base numerator `u32` |
| 32 | movie time-base denominator `u32` |
| 36 | required feature word `u64`, currently must be zero |
| 44 | optional feature word `u64` |
| 52 | max record hint `u64`, bounded by reader policy |
| 60 | first checkpoint offset `u64` |
| 68 | CRC32C over bytes 0 through 67 |

Time bases must be positive and reduced.

## Records

Record prefix:

```text
type:u16
flags:u16
header_size:u16
alignment_log2:u8
reserved:u8
payload_size:canonical ULEB128
sequence:canonical ULEB128
payload bytes
zero padding to alignment
crc32c over prefix+payload+padding
```

Known implemented records:

- `STREAM_DESC` (`0x0010`): track, generation, media kind, codec tag, time base, and config bytes.
- `PACKET` (`0x0020`): track, generation, sequence, DTS, PTS, duration, flags, and opaque payload bytes.
- `GROUP` (`0x0001`): group metadata followed by exact child record bytes.
- `PACKET_TABLE`, `EDIT_LIST`, `DISCONTINUITY`, `SEEK_INDEX`, `CHECKPOINT`, `INDEX_DIRECTORY`, and `FOOTER`: framed and bounded; detailed payload semantics are still growing.

Unknown records with the high bit set are skippable. Unknown required records fail with `TIDE_STATUS_UNSUPPORTED`.

## Partial Files

EOF inside a record returns `TIDE_STATUS_PARTIAL` and exposes the last complete prefix through `tide_decoder_valid_prefix` or `tide_repair_plan_valid_prefix`.
