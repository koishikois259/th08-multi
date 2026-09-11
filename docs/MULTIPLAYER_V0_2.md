# th08-multi v0.2 release checkpoint

This release records the locally tested multiplayer implementation after the
initial v0.1 packaging milestone.

## Included behavior

- P1 and P2 independently select any of the four human/youkai teams, including
  the same team.
- Lives, bombs, power, and the human/youkai gauge are per-player resources.
- Score, point-item count, maximum point value, time points, and Last Spell
  activation are shared.
- Power/life/bomb items belong to the player who collects them; point-resource
  items choose the nearer living player while updating the shared resource.
- Graze collision is calculated per player and the displayed graze count is
  the combined total.
- Boss targeting chooses the nearer living player.
- The run ends only when both players can no longer continue. A defeated
  player becomes a moving spirit that can be revived; its initial direction
  is randomized away from directions perpendicular to a playfield edge, and
  it reflects at the playfield boundary.
- The HUD labels each player's `health`, `bomb`, and `power`, while shared
  resource values are displayed once and identically for both peers.
- Overlapping players fade the non-local character to reduce visual obstruction.
- Character-specific spell portraits/effects, focus hitboxes, boss dialogue,
  and stage transitions use the two-player runtime paths.

## Distribution and compatibility

The Windows patch is data-free and supports only the Japanese TH08 1.00d
executable identified by SHA-256
`330fbdbf58a710829d65277b4f312cfbb38d5448b3df523e79350b879213d924`.
Both peers must run the same v0.2 build. Replay recording/playback, relay,
matchmaking, UPnP, and NAT traversal are not included.
