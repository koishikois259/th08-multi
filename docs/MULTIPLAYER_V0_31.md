# th08-multi v0.31 release checkpoint

v0.31 is a rendering and resource-lifetime stability update built on v0.3.

## Fixes

- Dialogue portrait VMs are validated against their current owning face ANM;
  stale VMs are detached before later-stage face resources are released.
- ANM slots detach their pointer ownership before freeing the underlying data,
  preventing re-entrant or aliased release paths from freeing one allocation
  twice.
- P1/P2 shared player resources use explicit borrower/owner teardown ordering.
- Child ECL allocations have one centralized release path and duplicate owners
  are detached before cleanup.
- Stage background, enemy, and ECL references are detached before transition
  teardown, preventing later stages from retaining pointers to freed data.
- Multiplayer dialogue clears and redraws the frozen playfield every frame,
  removing portrait and transition trails in stages 4, 5, 6, and Extra without
  advancing gameplay simulation.

## Compatibility

Both peers must use the same v0.31 patch. The gameplay build fingerprint and
launcher handshake version are advanced to `0x00030001`, so v0.31 cannot be
mixed with v0.3 or earlier builds. The supported base game remains Japanese
TH08 1.00d.

## Validation boundary

The defensive release paths compile and the original non-multiplayer rendering
functions remain byte-exact. The stage 4/5/6/Extra dialogue-trail fix was
accepted in a local two-process gameplay test. Because the original remote
crash dump is unavailable, the earlier `ntdll!free` crash cannot yet be proven
closed until a longer two-peer later-stage run completes without the same WER
signature.

## Local release artifact

The data-free `th08-multi-v0.31-patch.zip` was generated and installed into the
verified local `th08-origin` directory. Its SHA-256 is recorded in
`MULTIPLAYER_VALIDATION.md`.
