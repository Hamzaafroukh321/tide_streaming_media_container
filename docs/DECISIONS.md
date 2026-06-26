# Architecture Decisions

## ADR-001: Use A Small Self-Contained C Test Harness

Context: The specification requires production-linked unit, integration, and malformed-input tests, but the repository must not depend on network downloads during normal builds.

Decision: Tests use a tiny first-party C harness with `TIDE_EXPECT` macros and CTest registration rather than downloading an external framework.

Alternatives considered: Unity, Criterion, or CMocka. These are useful, but adding them would require vendoring or download behavior that is unnecessary for the current scope.

Consequences: Test code stays portable and reviewable. Assertions are basic, so richer diagnostics must be written explicitly.

Validation: CTest targets will be registered for each test executable.

## ADR-002: Keep Production Dependencies At The C Standard Library And POSIX Boundary

Context: Tide's core work is container parsing, bounds checking, timeline math, ownership, and repair behavior. Dependencies must not replace that core work.

Decision: The implementation uses only the C standard library for the portable core and isolates optional POSIX file access behind source adapters. CRC32C and the digest adapter are first-party implementations.

Alternatives considered: external checksum/hash libraries or media container libraries. These would add license and behavior drift risk.

Consequences: The digest adapter is suitable for deterministic integrity checks in this project but must not be described as a cryptographic BLAKE3 implementation.

Validation: Dependency inventory in documentation remains empty for production code.

## ADR-003: Treat Missing Local Toolchain As Verification Blocker, Not A Design Deviation

Context: The current Windows environment lacks CMake and C compilers on PATH.

Decision: Maintain normal CMake/Linux build files and record local verification as blocked until a supported C17 toolchain is available.

Alternatives considered: rewriting the project in a locally available language or claiming unrun tests. Both violate the specification.

Consequences: Source, tests, and docs can be prepared, but build/sanitizer/fuzz verification must be run later on a proper toolchain.

Validation: `docs/IMPLEMENTATION_STATUS.md` lists exact commands and blocker details.
