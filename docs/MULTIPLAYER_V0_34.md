# th08-multi v0.34 release checkpoint

v0.34 is a local patch based on v0.33. It has not been uploaded to GitHub.

## Changes

- The Host/P1 configuration is authoritative for both players' initial lives.
  The Guest/P2 local initial-lives setting is ignored. The default remains 2.
- The remote character renders at 50% opacity when the two characters are at
  least 64 pixels apart and 10% when closer. The remote white focus hitbox is
  not rendered in the local player's view.
- The right sidebar has separate P1 and P2 LIFE/BOMB rows. Normal-size (1.0x)
  red and blue ASCII star marks use the same visible draw path as the other
  sidebar text. The user confirmed the marks are visible at the previous 0.9x
  scale; the final size still needs a visual check.
- The bundled Chinese connection guide now states the initial-lives rule.

## Compatibility and distribution

Both peers need the same v0.34 patch and Japanese TH08 1.00d game data. The
gameplay build fingerprint and launcher handshake version are `0x00030004`,
so mixed versions are rejected. The seven-file patch excludes the original
game executable, DAT files, save data, and replay data. The ZIP is installed
into the verified local `th08-origin` directory without replacing `th08.exe`.
