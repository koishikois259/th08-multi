# th08-multi v0.3 release checkpoint

v0.3 is a gameplay-stability update built on v0.26.

## Fixes

- P1 and P2 may both select the Border Team. Identical teams share the
  immutable P1 player ANM resources while keeping independent animation VMs,
  preventing the second load from invalidating P1 sprite data.
- Multiplayer dialogue draws only the currently active portrait VM, preventing
  stale portrait layers from remaining visible during later-stage dialogue.
- Lockstep input no longer advances on locally skipped presentation frames.
  Frame skipping varies by machine and previously caused the peers to apply
  different subsets of P1/P2 input before the desync guard disconnected them.

## Compatibility

Both peers must use the same v0.3 patch. Gameplay and launcher fingerprints
are advanced to v0.3, preventing accidental mixed-version sessions with older
patches. The supported base game remains Japanese TH08 1.00d.

## Local release artifact

The data-free `th08-multi-v0.3-patch.zip` was generated and installed into the
verified local `th08-origin` directory with SHA-256
`637cc302e092f730eae7f464bb7c0c33d809c09f9db729da71a8808de622a9f9`.
