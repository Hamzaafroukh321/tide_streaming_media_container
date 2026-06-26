# Fuzzing

The repository defines three harnesses:

- `fuzz_decoder`: feeds bytes to `tide_decoder_feed`.
- `fuzz_chunk_boundaries`: compares production decoder status and event counts for contiguous input versus a split input plan.
- `fuzz_demux_remux`: opens accepted data with production demux, rewrites it with production mux, reopens the result, and checks stream/packet counts.

Build:

```sh
cmake --preset fuzz
cmake --build --preset fuzz
```

Smoke commands:

```sh
./build/fuzz/fuzz_decoder corpus/decoder/minimal.tide
./build/fuzz/fuzz_chunk_boundaries
./build/fuzz/fuzz_demux_remux
```

When libFuzzer is enabled, run:

```sh
./build/fuzz/fuzz_decoder -runs=1000 corpus/decoder
./build/fuzz/fuzz_chunk_boundaries -runs=1000 corpus/chunked
./build/fuzz/fuzz_demux_remux -runs=1000 corpus/roundtrip
```

Standalone smoke commands used on Windows:

```powershell
.\example_write_capture.exe
.\fuzz_decoder.exe example.tide
.\fuzz_chunk_boundaries.exe
.\fuzz_demux_remux.exe
```

Failures should be minimized, added as normal regression tests, and recorded in the traceability matrix. Fuzz inputs are untrusted data and must never be executed as commands.
