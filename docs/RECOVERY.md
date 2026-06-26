# Recovery

The repair scanner uses the production decoder to find the longest complete prefix. It accepts a full valid file as `TIDE_STATUS_OK` and a truncated tail as `TIDE_STATUS_PARTIAL`.

`tide_repair_write` demuxes only bytes through `tide_repair_plan_valid_prefix` and writes a fresh TIDE file through the normal mux path. It does not overwrite the source path, and the repaired output is expected to validate through the normal demux path.

Future recovery work must add authenticated checkpoints, digest fallback, rebuilt indexes, and richer omission reporting before the full-version repair criteria can be marked verified.
