# Testing

Tests are first-party C executables registered through CTest. They link `tide_core` and construct fixtures through the production mux/writer path.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Covered test names include:

- `HeaderCanonicalRoundTrip`
- `RationalExactConversion`
- `NearestEvenTieCases`
- `TimestampInvalidBaseRejected`
- `EditClipProducesExpectedPieces`
- `EditGapDropsPacket`
- `ReorderDepthRelease`
- `PartialTailStatus`
- `DemuxRemuxPayloadIdentity`
- `RepairDropsIncompleteGroup`

Additional malformed, allocation-failure, cancellation, compatibility, and long soak tests remain required before full-version completion.
