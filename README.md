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

On Windows, this repository has also been verified with explicit Clang/Ninja paths:

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build\debug-ninja -G Ninja -DCMAKE_MAKE_PROGRAM='C:/Users/Hamz/AppData/Local/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe' -DCMAKE_C_COMPILER='C:/Program Files/LLVM/bin/clang.exe' -DCMAKE_RC_COMPILER='C:/Program Files/LLVM/bin/llvm-rc.exe' -DCMAKE_BUILD_TYPE=Debug
& 'C:\Program Files\CMake\bin\cmake.exe' --build build\debug-ninja
& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build\debug-ninja --output-on-failure
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

Current verification:

- Debug and Release builds pass with CMake/Ninja/Clang on Windows.
- CTest passes for Debug, Release, ASan/UBSan runtime, and the non-instrumented TSan-profile fallback.
- Fuzz harness smoke commands pass for decoder, chunk-boundary, and demux-remux entry points.
- ThreadSanitizer instrumentation is not supported by the installed Windows Clang target.

See `docs/IMPLEMENTATION_STATUS.md` and `docs/REQUIREMENTS_TRACEABILITY.md` for the precise status.
