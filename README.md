# Tide Streaming Media Container Toolkit

Tide is a C17 toolkit for a small original streaming media container. It provides a bounded TIDE-1 decoder, payload ownership through explicit leases, rational timestamp helpers, sequential demux/remux workflows, partial-tail repair scanning, a CLI, tests, and fuzz harnesses.

The project is early implementation work against `04_tide_streaming_media_container.md`. It is not a stable ABI or production release yet.

## Build

Requires a C17 compiler and CMake 3.24 or newer.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Sanitizer profiles:

```sh
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
```

## CLI

```sh
tide inspect capture.tide
tide demux capture.tide
tide remux capture.tide rewritten.tide
tide index capture.tide
tide repair broken.tide repaired.tide
```

`inspect`, `demux`, and `index` currently validate and summarize the container through the production demux path. `remux` rewrites descriptors and packet payloads into a fresh canonical TIDE file. `repair` writes the validated prefix reported by the production decoder.

## Status

Implemented:

- C17 build files and presets.
- TIDE-1 header and record writer.
- Header, record, CRC, padding, descriptor, packet, unknown-required, and partial-tail decoder checks.
- Memory/file sources and copied payload leases.
- Rational timestamp conversion, edit clipping helper, deterministic reorder queue.
- Sequential demux/remux and prefix repair.
- Unit/integration tests and three fuzz harness entry points.

Blocked locally:

- Native build, CTest, sanitizer, and fuzz smoke verification because this environment has no CMake or C compiler on `PATH`.

See `docs/IMPLEMENTATION_STATUS.md` and `docs/REQUIREMENTS_TRACEABILITY.md` for the precise status.
