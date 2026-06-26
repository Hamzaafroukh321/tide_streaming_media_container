# Architecture

Tide is organized as a single portable C core with narrow adapters around files and command-line workflows.

## Modules

- `src/common`: checked arithmetic, limits, and stable errors.
- `src/format`: TIDE-1 primitive reader, CRC32C, decoder, and writer.
- `src/source`: memory/file sources.
- `src/model`: payload leases, packet moves, rational time, edit projection.
- `src/index`: deterministic reorder queue and provisional index entry builder.
- `src/demux`: source-to-packet workflow using production decoder callbacks.
- `src/mux`: canonical header, stream, packet, and footer writer.
- `src/repair`: production-decoder prefix scan and repair-prefix writer.
- `src/cli`: thin command dispatch.

## Ownership

Public packet payloads are retained by `tide_payload_lease`. `tide_packet_ref_move` transfers a packet and clears the source. Demux copies accepted packet payloads into leases so callback-local decoder spans are never retained as raw pointers.

Decoder, demux, mux, reorder queue, repair scanner, and mutable index builder are single-owner objects. Payload lease reference counts are atomic because leases may eventually cross worker boundaries.

## State

The decoder accumulates bytes until final feed and then validates the file through one parser path. It records the last complete prefix. On EOF inside a record, it returns `TIDE_STATUS_PARTIAL`; on structural or integrity failure, it returns a typed error.

Mux starts active after writing the header, accepts stream descriptors and packets, and closes by writing a footer before publishing bytes to disk. Abort closes and releases buffered data without claiming a valid output.

Repair uses the same decoder path as normal parsing. It never scans blindly for magic inside payload bytes.

## Locking

The current implementation is single-threaded except for atomic payload lease reference counts. No public object except leases is thread-safe.

## Invariants

- Integer and offset calculations use checked helpers before allocation or slicing.
- Record padding must be zero and CRC32C must match before callbacks receive records.
- Required unknown records are rejected; high-bit skippable records are consumed exactly.
- Packet records require a matching stream descriptor generation.
- Remux never mutates payload bytes.
