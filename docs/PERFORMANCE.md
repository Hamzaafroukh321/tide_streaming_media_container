# Performance

No numeric performance measurements have been run in the current environment because no CMake or C compiler is available on `PATH`.

## Budgets From The Specification

- MVP parse throughput: at least 500 MiB/s payload-heavy.
- MVP memory: at most 128 MiB default.
- MVP track count: 64 tracks.
- MVP record/payload: 64 MiB record policy.
- MVP fuzz speed: more than 30k decoder executions/s and 3k chunk plans/s.

## Commands

```sh
cmake --preset release
cmake --build --preset release
./scripts/benchmark.sh
```

Benchmark scripts and stable corpora still need to be added before these budgets can be verified.
