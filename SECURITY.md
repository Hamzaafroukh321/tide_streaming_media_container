# Security

Tide treats all input bytes as untrusted local data.

## Supported Versions

Only TIDE major version `1` is currently recognized.

## Threat Model

Attackers may provide malformed, truncated, very large, or adversarially structured local files. They should not be able to trigger out-of-bounds reads, unchecked allocation growth, integer overflow, use-after-free, double release, or arbitrary command execution.

## Defensive Rules

- Validate offsets, lengths, depths, feature bits, padding, and CRC before emitting records.
- Keep payload bytes opaque.
- Do not trust footer or index offsets as authority.
- Report only validated IDs, offsets, and record types in diagnostics.
- Bound allocations through `tide_limits`.

## Disclosure

This repository does not yet define a public vulnerability disclosure channel. Report issues to the repository owner.
