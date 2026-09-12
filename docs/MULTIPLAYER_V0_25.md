# th08-multi v0.25 release checkpoint

v0.25 is a focused stability update built on v0.24.

## Extra-stage crash fix

- Fixed the reproducible access violation that occurred in Extra when the GUI
  attempted to copy an enemy-name texture through a released or not-yet-ready
  stage-text sprite allocation.
- Multiplayer now validates the complete source/destination sprite span before
  copying the name texture. If the stage transition still owns the resource,
  the copy is deferred and retried on a later GUI update instead of
  dereferencing stale memory.
- Windows Error Reporting identified the previous failure as exception
  `0xc0000005` at module offset `0x0002a8ec`, inside
  `Gui::CopyEnemyNameTexture`. Both local peers failed at the same instruction.

## Compatibility

Both peers must use the same v0.25 patch. Gameplay and launcher fingerprints
are advanced to v0.25, preventing accidental mixed-version sessions with
v0.24. The supported base game remains Japanese TH08 1.00d.

## Local release artifact

`th08-multi-v0.25-patch.zip` was built and installed locally with SHA-256
`17b880425f0f529d8ab65304002baaba11f66b83c312fbfcd50361ecd7cc5c17`.
The archive contains no original executable, DAT archive, save, or replay data.
