# Fuzzing

The repository defines three harnesses:

- `fuzz_decoder`: feeds bytes to `tide_decoder_feed`.
- `fuzz_chunk_boundaries`: exercises decoder event recording through alternate byte plans.
- `fuzz_demux_remux`: opens accepted data with production demux and rewrites it with production mux.

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

Failures should be minimized, added as normal regression tests, and recorded in the traceability matrix. Fuzz inputs are untrusted data and must never be executed as commands.
