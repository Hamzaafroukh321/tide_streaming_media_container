# Requirements Traceability

| ID | Specification section | Requirement | Implementation files | Test/fuzz evidence | Status | Commit |
|---|---|---|---|---|---|---|
| REQ-001 | 1, 17 | C17 CMake project with core, CLI, tests, tools, and fuzz targets. | `CMakeLists.txt`, `cmake/*` | Configure/build/CTest pending | Not started | TBD |
| REQ-002 | 12 | Stable status/error categories with offset/path context. | `include/tide/error.h`, `src/common/error.c` | Error tests pending | Not started | TBD |
| REQ-003 | 12, 18 | Checked arithmetic and resource limits before allocation, slicing, and timestamp conversion. | `include/tide/limits.h`, `src/common/checked.c` | Boundary tests pending | Not started | TBD |
| REQ-004 | 7 | TIDE-1 header validation including magic, version, feature bits, time base, UUID, and CRC. | `src/format/decoder.c`, `src/format/writer.c` | Header tests pending | Not started | TBD |
| REQ-005 | 7, 8 | Incremental record decoder with bounded explicit group/frame stack and partial-tail status. | `include/tide/decoder.h`, `src/format/decoder.c` | Split and partial tests pending | Not started | TBD |
| REQ-006 | 7 | ULEB128, little-endian primitive, padding, CRC, unknown-record policy, and canonical writer. | `src/format/reader.c`, `src/format/writer.c` | Round-trip/malformed tests pending | Not started | TBD |
| REQ-007 | 6, 9 | Zero-safe lifecycle, opaque handles, track generations, packet refs, and payload leases. | `include/tide/tide.h`, `src/model/*`, `src/source/*` | Ownership tests pending | Not started | TBD |
| REQ-008 | 7 | STREAM_DESC, PACKET, PACKET_TABLE, EDIT_LIST, DISCONTINUITY, SEEK_INDEX, CHECKPOINT, INDEX_DIRECTORY, FOOTER records. | `src/format/records.c`, `src/model/*` | Record tests pending | Not started | TBD |
| REQ-009 | 10 | Rational timestamp conversion with floor, ceil, and nearest-even rounding without overflow. | `src/model/time.c` | Time tests pending | Not started | TBD |
| REQ-010 | 10 | Canonical edit lists and packet interval projection. | `src/model/edit.c` | Edit tests pending | Not started | TBD |
| REQ-011 | 10, 13 | Bounded deterministic reorder queue, discontinuities, and explicit single-owner thread model. | `src/index/reorder.c` | Reorder tests pending | Not started | TBD |
| REQ-012 | 10 | Provisional seek-index builder and validation fallback. | `src/index/index_builder.c` | Index tests pending | Not started | TBD |
| REQ-013 | 11 | Public API for decoder, demux, mux, repair, and packet ownership. | `include/tide/*.h`, `src/demux/*`, `src/mux/*`, `src/repair/*` | API/integration tests pending | Not started | TBD |
| REQ-014 | 11 | CLI commands `inspect`, `demux`, `remux`, `index`, and `repair` with documented exit codes. | `src/cli/*` | CLI tests pending | Not started | TBD |
| REQ-015 | 10, 21 | Canonical remux preserving selected payload bytes/timestamps and reopening successfully. | `src/mux/*` | Remux integration pending | Not started | TBD |
| REQ-016 | 10, 21 | Checkpoint-led strict repair scanner and fresh repaired output. | `src/repair/*` | Repair tests pending | Not started | TBD |
| REQ-017 | 14, 15 | Decoder fuzz harness bounded and production-linked. | `fuzz/fuzz_decoder.c` | Fuzz smoke pending | Not started | TBD |
| REQ-018 | 14, 15 | Chunk-boundary fuzz harness comparing split and contiguous decode. | `fuzz/fuzz_chunk_boundaries.c` | Fuzz smoke pending | Not started | TBD |
| REQ-019 | 14, 15 | Demux-remux fuzz harness using production demux/mux and model checks. | `fuzz/fuzz_demux_remux.c` | Fuzz smoke pending | Not started | TBD |
| REQ-020 | 16 | Unit, integration, malformed, truncation, cancellation, allocation-failure, and deterministic tests. | `tests/*` | CTest pending | Not started | TBD |
| REQ-021 | 18 | Performance budget commands and honest unverified/verified measurements. | `bench/*`, `docs/PERFORMANCE.md` | Benchmarks pending | Not started | TBD |
| REQ-022 | 19, 24 | Required documentation deliverables match actual behavior. | `README.md`, `docs/*`, `FUZZING.md`, `TESTING.md` | Documentation review pending | In progress | TBD |
| REQ-023 | 23 | Assisted development provenance is transparent. | `ASSISTED_DEVELOPMENT.md` | Document present | In progress | TBD |
