# th08-multi v0.24 release checkpoint

v0.24 is a focused update built on v0.23.

## Gameplay

- A player revived from spirit state now returns at the maximum Power value of
  128 instead of restarting at zero Power.
- The full-Power value is written into the deterministic multiplayer state and
  synchronized with P1's retail global Power storage, so the rule is identical
  when either P1 or P2 is revived.

## Launcher

- The informational `Local IPv4:` row was removed from the launcher because it
  could omit virtual adapters or otherwise suggest the wrong address.
- The explicit `Bind local IPv4` field remains. Players should enter the IPv4
  assigned to the LAN or trusted-VPN adapter they intend to use.

## Compatibility

Both peers must use the same v0.24 patch. Gameplay and launcher fingerprints
are advanced to v0.24, preventing accidental mixed-version sessions with
v0.23. The supported base game remains Japanese TH08 1.00d.

## Local release artifact

`th08-multi-v0.24-patch.zip` was built and installed locally with SHA-256
`58f18aa489fac430e87febcf9967d46c276c4b2d57d202f37ea01b78b55c8e6c`.
The archive contains no original executable, DAT archive, save, or replay data.
