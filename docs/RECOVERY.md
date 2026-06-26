# Recovery

The repair scanner currently uses the production decoder to find the longest complete prefix. It accepts a full valid file as `TIDE_STATUS_OK` and a truncated tail as `TIDE_STATUS_PARTIAL`.

`tide_repair_write` writes only bytes through `tide_repair_plan_valid_prefix` to a new path. It does not overwrite the source path.

Future recovery work must add authenticated checkpoints, digest fallback, rebuilt indexes, and fresh footer validation before the full-version repair criteria can be marked verified.
