# th08-multi v0.23 release checkpoint

v0.23 collects the gameplay, rendering, stability, and trusted-network safety
work completed after v0.22.

## Gameplay and stability

- P1 and P2 may select the same team without loading the same immutable player
  animation resources twice or crashing during startup.
- Extra is available in multiplayer without depending on either peer's local
  `score.dat` unlock state.
- The network session continues to be serviced while the next stage is loaded,
  preventing a slow remote stage transition from timing out as a crash.
- A shared Last Spell failure is owned by the first colliding player. The other
  living player is protected immediately, preventing a second same-frame
  failure callback from corrupting the shared spell and boss state.
- The standard laser direction helper is restored in multiplayer builds. Laser
  bodies and cancellation positions now use initialized sine and cosine values,
  covering Keine's history spell, Marisa's third spell, Kaguya's Dragon's Neck
  Jewel and Fire Rat's Robe patterns, and every other standard laser user.

## Visibility and HUD

- A nearby remote player fades to 25% opacity, and that player's Focus aura is
  hidden at close range so it does not obscure the local hitbox.
- Graze flashes and Time-item score popups are suppressed in multiplayer to
  reduce bullet-field clutter without changing their score or resource effects.
- The multiplayer sidebar no longer force-draws overlapping decorative
  `front.anm` artwork. It keeps the basic labelled P1/P2 resource rows, shared
  values, score, high score, boss name, and boss life display.

## Trusted LAN/VPN safety

- Bind addresses must be explicit local IPv4 addresses; wildcard binding and
  direct public-Internet use remain unsupported.
- Session packets are validated and pinned to the negotiated peer endpoint,
  with bounded packet processing and stronger session nonces/seeds.
- The deterministic malformed-packet corpus was expanded, and sanitizer-backed
  protocol fuzzing is available in CI.
- Packaging rejects original game data and emits both per-file checksums and an
  external ZIP checksum.

## Compatibility

Both peers must use the same v0.23 patch. Gameplay and launcher fingerprints
are advanced to v0.23, so v0.22 and v0.23 cannot connect accidentally. The
supported base game remains Japanese TH08 1.00d. Replay, public matchmaking,
relay, automatic port forwarding, and NAT traversal are not included.

## Local release artifact

`th08-multi-v0.23-patch.zip` was built and installed locally with SHA-256
`5e86719a975ccbcaf6c9efba1503c8f032c47bdca7cd9866fb4e353560b17e2c`.
The archive contains no original executable, DAT archive, save, or replay data.
