# th08-multi v0.22 release checkpoint

v0.22 builds on the synchronized pre-game launcher introduced in v0.21 and
includes the gameplay and presentation fixes accepted during local two-process
testing.

## Gameplay fixes

- Continuing after both players are defeated restores both players to their
  initial life, bomb, and human-youkai resources, with Power set to 128.
- A player who remains a spirit across a stage boundary can still be revived;
  stage transitions do not incorrectly restore an exhausted player.
- A shared Point extend awards a life only to players who are currently alive.
- The Stage 4 boss route is determined by P1's team.
- The Stage 5 final-route selection accepts movement and confirmation only
  from P1.

## Presentation fixes

- Boss dialogue, player focus markers, bomb effects, and player-specific bomb
  portraits remain available in co-op.
- Boss life bars are restored for every phase, not only the first health bar.
- The original boss-name plate and right-side `front.anm` artwork are kept in
  the live multiplayer render path.
- The multiplayer HUD labels independent Life, Bomb, Power, Graze, and
  human-youkai resources for both players while keeping shared values common.

## Compatibility

Both peers must use the same v0.22 patch. The launcher and gameplay build
fingerprints were advanced to v0.22, preventing mixed v0.21/v0.22 sessions.
The supported base game remains Japanese TH08 1.00d. Replay, matchmaking,
relay, automatic port forwarding, and NAT traversal are not included.
