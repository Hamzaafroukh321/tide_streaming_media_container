# Tide Implementation Plan

## Selected Specification

`04_tide_streaming_media_container.md` is the governing specification. It matches the repository name and contains the complete numbered architecture, fuzzing, MVP, and full-version acceptance sections required by the task prompt.

## Architecture Summary

Tide is a C17 streaming media container toolkit. The implementation is organized around:

- bounded byte readers, checked arithmetic, and typed diagnostics;
- an incremental TIDE-1 decoder with an explicit frame stack;
- catalog, packet, lease, time, edit, reorder, index, demux, mux, and repair modules;
- a CLI for `inspect`, `demux`, `remux`, `index`, and `repair`;
- production-linked tests and fuzz harnesses.

The core invariant is that untrusted bytes never become offsets, slices, timestamps, or retained payloads until they pass explicit bounds, version, integrity, and resource-limit checks.

## Phases

1. Foundation: CMake, C17 targets, warning/sanitizer presets, test harness, allocator and status primitives.
2. Data model: zero-safe lifecycle, handles, payload leases, source adapters, catalog descriptors.
3. Format: header, ULEB128, record framing, GROUP, descriptors, packets, tables, edits, indexes, checkpoints, footer.
4. Timeline and packet flow: rational conversion, edits, reorder, demux, selection.
5. Writing and repair: canonical writer, remux, checkpoints, strict repair scan, fresh repaired output.
6. Hardening: cancellation, fault injection, fuzz harnesses, malformed corpus, sanitizer passes.
7. Performance and docs: benchmark commands, budgets, compatibility fixtures, final traceability audit.

## Dependency Graph

Foundation -> Data model -> Format -> Timeline -> Demux -> Mux -> Index -> Repair -> Concurrency/cancellation -> Fuzz/performance/docs.

## Requirement Groups

- RG-FND: repository, CMake, tests, sanitizer/fuzz profile setup.
- RG-ERR: error/status model, checked arithmetic, resource limits.
- RG-FMT: TIDE-1 header, records, CRC, ULEB128, grouping, unknown handling, partial tails.
- RG-MDL: handles, source, leases, catalog, packet refs, lifecycle.
- RG-TIME: rational conversion, edit lists, projection, rounding.
- RG-FLOW: reorder, demux, selection, index building.
- RG-WRITE: mux, remux, footer, checkpoints, repair.
- RG-HARD: cancellation, malformed inputs, allocation/I/O faults, fuzzing.
- RG-DOC: architecture, format, testing, fuzzing, security, recovery, performance, traceability.

## Validation Commands

Expected Linux/POSIX toolchain commands:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
cmake --preset release
cmake --build --preset release
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/fuzz_decoder -runs=1000
./build/fuzz/fuzz_chunk_boundaries -runs=1000
./build/fuzz/fuzz_demux_remux -runs=1000
```

Current local blocker: this Windows environment has no `cmake`, `cc`, `gcc`, `clang`, `cl`, or `ninja` on `PATH`.

## Risks And Mitigation

- Format ambiguity: keep `docs/TIDE_FORMAT.md` synchronized with writer/decoder tests.
- Integer overflow: centralize checked operations and use them before allocation/slicing.
- Ownership defects: explicit lease retain/release and move tests.
- Partial-file confusion: distinguish `TIDE_STATUS_PARTIAL` from success and invalid input.
- Toolchain absence: keep source buildable and record exact verification gaps until a C17 toolchain is available.

## Definition Of Done

All requirements in `docs/REQUIREMENTS_TRACEABILITY.md` are `Verified` or explicitly `Blocked`, clean debug/release/sanitizer builds pass, tests and fuzz smoke runs pass, docs match behavior, and the working tree is clean except documented user-owned files.

## Checklist

- [ ] MVP: incremental decoder and split-independent record events.
- [ ] MVP: descriptors, packets, packet tables, edits, indexes, checkpoints, footer bounds.
- [ ] MVP: demux exact payload bytes and timestamps with lease ownership.
- [ ] MVP: canonical remux reopens and preserves selected packet semantics.
- [ ] MVP: partial EOF exposes only complete prefix.
- [ ] MVP: malformed offset/length/time/CRC/unknown-required failures.
- [ ] MVP: three fuzz targets build and smoke-run.
- [ ] Full: reorder, index, discontinuity, rich edits, interleaving, repair.
- [ ] Full: worker handoff/cancellation/shutdown and TSan where supported.
- [ ] Full: performance budgets and final go/no-go audit.
