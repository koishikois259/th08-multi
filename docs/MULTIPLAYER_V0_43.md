# th08-multi v0.43 local patch checkpoint

Prepared on October 17, 2026, from the stage-loading fixes in `8d77cf45`.
The patch is local only: no GitHub upload or tag. After packaging, it was
installed into `../th08-origin` at the user's request, as recorded below.

## Changes

- The first simulation tick after loading uses one consumed network-frame
  decision for readiness, RNG reset, pre-tick state hash and gameplay counting.
- Title/stage readiness is separate from physical buttons. Holding R cannot
  advertise setup completion or bypass settings acknowledgement; in-flight
  readiness cannot become a reset key. Gameplay epochs reject stale readiness.
- Stage lifecycle transitions run on the main thread before starting the setup
  worker. The worker publishes completion with Interlocked after its final
  state write. Pending input remains immutable until consumed or disconnected.
- Game, title-screen and launcher window titles now identify v0.43. The English
  README and packaged Chinese README describe this release. The existing
  Chinese tutorial is preserved without rewriting or reimporting it.

The implementation and remaining investigation limits are recorded in
[the stage-loading audit](MULTIPLAYER_STAGE_LOAD_FIX_HANDOFF_20261017.md).

## Compatibility and archive layout

Wire version is `0x00010004`; the game fingerprint and launcher gate are
`0x00040004`. Both players must update both executables. Published v0.42
(`0x00040003` gate, `0x00010003` wire) and older patches are incompatible.

The ZIP has exactly the same seven root entries as the existing local v0.42
archive, without a containing directory:

```text
SHA256SUMS.txt
th08-multi-LICENSE.txt
th08-multi-README.txt
th08-multi-launcher.exe
th08-multi.exe
th08_multi.ini.example
联机教程.txt
```

No original game files, saves, active INI/config, logs or test executables are
included. The original v0.42 ZIP and checksum remain untouched. Output files:
`../th08-multi-v0.43-patch.zip` and its `.sha256` sidecar. Rebuild and package:

```powershell
python scripts/build.py --build-type multi -j 1 build/th08-multi.exe build/th08-multi-launcher.exe multiplayer-tests
python scripts/package-multiplayer-patch.py
```

## Validation

The VC7 multiplayer game/launcher build and all eight native multiplayer suites
pass. The coordinator suite includes paired real-UDP sessions with independent
engine-adapter states, swapped fast/slow loaders, held R, two loading cycles and
input delays 1/3/12. Both peers start on the same consumed network frame and
their RNG/gameplay counters agree through 40 simulated ticks per cycle.
These tests do not replace full two-PC gameplay, actual resource loading or
long runs; they do not establish that every reported error=7 is resolved.

Packaging checks cover exact archive entry/order equality with v0.42, ZIP CRC,
source-to-ZIP byte equality for all payload files, internal/external checksums,
v0.43 executable title strings and unchanged tutorial bytes. The tutorial is
compared with the old v0.42 archive and the workspace/docs/installed copies.
The packaging validation did not launch or install the game.

## Local installation

Installed into `../th08-origin` on October 17 at the user's request. All seven
installed files were verified byte-for-byte against the release ZIP. Four
files changed: the game, launcher, patch README and checksum manifest. The
tutorial, license and example INI already matched and were left untouched.

The previous seven patch files were backed up with their timestamps under
`build/origin-backup-v0.43-20261017-195335-a104a038/`. Its
`installation-verification.json` records the archive hash, installed file hashes
and preservation checks. Original executable/data, saves, active INI/config,
tutorial, custom/replay tools and log (ten files) retain their SHA-256 hashes
and modification times. No game process was running, and no game was launched
during installation.

## Repository checks

Documentation and whitespace checks pass. Full `scripts/ci.py` remains blocked
by the local WSL installation lacking `/bin/bash`; all other original CI checks
pass with only that unavailable check skipped in private analysis. No original
reconstruction layout, ledger or exactness claim is changed by versioning.

## Checksums

Unchanged tutorial SHA-256:
`c3ca2f2d56e922c2744aac5edad88e5a0d369bb152f52d715f08ca4b84150992`

Unchanged v0.42 archive SHA-256:
`0527391a7e704f3ac848b93f6e8d98d1a67c0f13956827ca6997ab8a4806ad98`

v0.43 archive SHA-256:
`fbbca2404d7b10092975aee7d583b55128b586462b1dd522134a861f1db6485a`
