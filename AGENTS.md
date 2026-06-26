# Repository Instructions

## Build

```sh
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

## Test

```sh
ctest --preset debug --output-on-failure
ctest --preset asan-ubsan --output-on-failure
```

## Sanitizers

```sh
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan --output-on-failure
```

## Fuzz

```sh
cmake --preset fuzz
cmake --build --preset fuzz
./build/fuzz/fuzz_decoder -runs=1000
./build/fuzz/fuzz_chunk_boundaries -runs=1000
./build/fuzz/fuzz_demux_remux -runs=1000
```

## Layout

- `include/tide`: public C ABI.
- `src/common`: errors, checked arithmetic, limits, allocator helpers.
- `src/format`: TIDE-1 reader, decoder, records, writer.
- `src/source`: memory/file/growing source adapters and payload leases.
- `src/model`: catalog, packets, time, edits.
- `src/index`: reorder queue and index builder/reader.
- `src/demux`, `src/mux`, `src/repair`: public workflows.
- `src/cli`: command-line tool.
- `tests`: production-linked tests.
- `fuzz`: libFuzzer-compatible harnesses.
- `docs`: architecture, format, recovery, traceability, and status.

## Conventions

Use C17, fixed-width serialized integers, checked arithmetic before every offset/size conversion, explicit ownership in names/docs, and one cleanup path per C function when resources are acquired.

## Safety

Never execute bytes from containers, corpora, or fuzz inputs. Never trust indexes or footers without validating record boundaries. Do not commit build products, corpora growth, coverage databases, logs, or local IDE state.

## Commit Expectations

Commit coherent, verified units with descriptive subjects and bodies listing context, implementation, invariants, and exact verification. Do not rewrite user history or use destructive Git commands.

## Definition Of Done

Requirements are verified or blocked in `docs/REQUIREMENTS_TRACEABILITY.md`, status is current, debug/release/sanitizer builds and tests pass where toolchains are available, fuzz smoke runs complete, docs match code, and the working tree is clean.
