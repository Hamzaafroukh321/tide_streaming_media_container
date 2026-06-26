# Contributing

Use the numbered specification and `PLANS.md` as the implementation guide.

## Style

- C17, fixed-width integer types for serialized values.
- Checked arithmetic before allocation, offset translation, and timestamp conversion.
- Explicit ownership transfer for packets and payload leases.
- One cleanup path in resource-acquiring functions.
- No production `TODO`, fake parser paths, or test-only substitute behavior.

## Workflow

1. Pick the next unblocked ticket from `docs/IMPLEMENTATION_STATUS.md`.
2. Add production behavior and focused tests in the same change.
3. Run the relevant build, CTest, sanitizer, and fuzz smoke commands.
4. Update traceability and status.
5. Commit a coherent unit with truthful verification details.

## Dependencies

Do not add a production dependency without an architecture decision explaining purpose, license, and why it does not replace Tide's core work.
