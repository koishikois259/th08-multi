# th08-multi v0.32 release checkpoint

v0.32 is a defensive memory-safety update built directly on v0.31. It targets
the fixed-size runtime arrays and heap-backed geometry most likely to corrupt
the heap before the repeatable later-stage `ntdll` free-path crash.

## Fixes

- Sound queue entries, sound indices, DirectSound buffer pointers, and playback
  status are validated before use.
- Radial-effect segment counts are bounded before allocating or writing dynamic
  trail vertices.
- ECL-controlled bullet transforms, laser slots, visible Boss slots, attached
  effects, and remote Boss operations reject invalid indices.
- Attached-effect consumers repair invalid counts and reject pointers outside
  the Effect pool before update or release.
- Enemy trail history, collision length, sample stride, and strip vertex count
  are checked at ECL setup, update, drawing, and trail-item emission.
- SHT files are checked for valid table and descriptor bounds before pointer
  relocation. Shot callbacks, option indices, power tables, persistent-shot
  timelines, and shot-trail lengths are validated again at runtime.

## Compatibility

Both peers must use the same v0.32 patch. The gameplay build fingerprint and
launcher handshake version are `0x00030002`, so v0.32 intentionally refuses to
connect to v0.31 or earlier builds. The supported base remains Japanese TH08
1.00d.

## Validation boundary

The VC7 multiplayer build and focused state, network, relocation-literal, and
semantic-protocol tests pass. The original non-multiplayer Player translation
unit also compiles with all v0.32 behavior excluded by `TH08_MULTI`.

A local two-process Host/Guest smoke run connected at input delay 3, selected
Border Team for both players, initialized both SHT files, and reached the
`playing (P1 team 0 / P2 team 0 / delay 3)` state without an SHT rejection.

The supplied dump identifies the final invalid heap operation, not the earlier
write that damaged the heap. These guards cover the highest-risk candidates but
do not prove the remote crash closed; that requires a sustained two-PC run
through stages 4, 5, 6, and Extra without the previous WER signature.

## Local release artifact

The data-free `th08-multi-v0.32-patch.zip` is generated and installed into the
verified local `th08-origin` directory. Its SHA-256 is recorded in
`MULTIPLAYER_VALIDATION.md`.
