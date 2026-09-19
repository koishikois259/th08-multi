# th08-multi v0.34 Online Multiplayer Patch

## 1. Project Nature and Rights Notice

th08-multi is an unofficial fan-made online multiplayer project based on *Touhou Eiyashou ~ Imperishable Night*. It is not affiliated with, authorized by, endorsed by, or developed in cooperation with Team Shanghai Alice, ZUN, or any official publisher or distributor. Please do not mistake this project for an official product or official update.

“Touhou Project,” *Touhou Eiyashou ~ Imperishable Night*, and all characters, artwork, music, text, and other original game content remain the property of their respective rights holders. This project does not grant users any right to copy or redistribute the original game content.

Users must legally obtain their own copy of the Japanese original version 1.00d of *Touhou Eiyashou ~ Imperishable Night*.

Touhou Project Fan Content Guidelines:
https://touhou-project.news/guidelines_en/

## 2. Supported Version

Only the Japanese original version 1.00d of *Touhou Eiyashou ~ Imperishable Night* is supported:

- `th08.exe` file size: 840704 bytes
- `th08.exe` SHA-256:
  `330fbdbf58a710829d65277b4f312cfbb38d5448b3df523e79350b879213d924`

Translated versions, trial versions, other releases, or modified game data are not supported.

Both players must use exactly the same v0.34 patch and the same version of the original game data.

## 3. Patch Contents and Distribution Boundaries

This patch archive contains only:

- `th08-multi.exe`
- `th08-multi-launcher.exe`
- `th08_multi.ini.example`
- `th08-multi-README.txt`
- `th08-multi-LICENSE.txt`
- `联机教程.txt`
- `SHA256SUMS.txt`

The patch does **not** include the original `th08.exe`, `th08.dat`, `thbgm.dat`, music, artwork, save data, or Replay files.

Do not repackage any of the above original game files, extracted original assets, or other users’ save data together with this patch. Do not distribute any pre-bundled or integrated game package containing such content.

`SHA256SUMS.txt` is provided for verifying the files included in the patch. The SHA-256 hash of the complete ZIP archive will be published on the corresponding GitHub Release page.

## 4. Installation and Startup

1. Back up your save data and configuration files.
2. Extract all files from the patch archive into the directory containing your legally obtained Japanese original version 1.00d of the game.
3. Keep the original `th08.exe`, `th08.dat`, and `thbgm.dat`. The patch does not overwrite `th08.exe`.
4. Both players should run `th08-multi-launcher.exe`.
5. The launcher displays the Local IPv4 address and does not provide a separate “Bind local IPv4” input field. The host should select **Host**. The guest should select **Guest** and enter the host’s LAN address, trusted VPN address, or address provided by a controlled tunneling service.
6. Both players must configure the same **Input delay**, then click **Connect**.
7. Once both sides show **Connected**, the Host should click **Start both games**. Both games will then launch from the same start command.

The game executable keeps title-menu input locked until both peers have
finished loading the main menu. P1 controls the shared mode and difficulty
selection after the synchronized menu state is reached.
At the start of a multiplayer run, both players use the Host/P1 initial-lives
setting; the Guest/P2 setting is ignored. The unchanged default is two.

GitHub source repository:
https://github.com/koishikois259/th08-multi

The local v0.34 patch has not yet been uploaded as a GitHub Release.

## 5. Network and Security Limitations

This project is intended only for two mutually trusted players using a trusted LAN, trusted VPN, or a temporary private tunneling environment with appropriate access controls.

The default port is UDP `17708`.

The protocol does not provide authentication, encryption, replay protection, or protection against a malicious peer. Therefore, the service should not be exposed directly to the public Internet, and users should not connect to untrusted players.

The launcher listens on all local IPv4 interfaces. Windows Firewall rules should be restricted to the intended network and trusted peer whenever possible.

When using a tunneling service, use only a temporary mapping, share the connection information only with the intended player, and disable the mapping after testing is complete. Connection addresses, public IP addresses, and tunnel ports may also constitute private or sensitive information.

## 6. Features and Known Limitations

- P1 and P2 may select teams independently.
- P1 and P2 may select the same team.
- Replay is currently not supported in online multiplayer mode.
- Connection and abnormal status information is displayed in the game window title bar.
- Diagnostic information is written to `log.txt` in the game directory.
- This project is still in the testing stage. Please keep backups of your save data and test primarily in trusted environments.

### Changes in v0.34

- Both players' initial lives follow the Host/P1 game setting, even when the
  Guest/P2 setting differs. The default remains two.
- The sidebar shows separate P1 and P2 LIFE and BOMB rows with larger red and
  blue star-shaped text marks. Other resource displays remain unchanged.
- The remote player's opacity is 50% when at least 64 pixels away and 10%
  when closer; the remote white focus hitbox is hidden from the local view.

### Major fixes inherited from v0.33

- P1 is authoritative for multiplayer clear progress, route availability, and
  unlock decisions; P2's local clear progress is ignored for the session.
- P1 progress is transferred as a validated bounded snapshot rather than as a
  raw `score.dat` file, and P2's disk save is not overwritten with P1 data.
- Title-menu input is blocked until both peers have reached the ready state;
  inputs held during loading must be released before menu control resumes.
- Multiplayer title-screen Demo playback is disabled because Replay remains
  unsupported.
- The gameplay RNG is reset at the common post-load simulation boundary, so
  different loading times or saves cannot split random bullet behavior.
- Desync hash identifiers now use the monotonic connection timeline, avoiding
  stale-hash collisions when starting another run without reconnecting.

v0.34 includes all defensive memory-safety checks introduced in v0.32. Long
two-PC runs through Stages 4, 5, 6, and Extra remain recommended.

## 7. Source Code, License, and Credits

The project source code is released under the MIT License contained in the repository root `LICENSE` file.

The MIT License applies only to source code and modifications that project contributors have the legal right to license. It does not apply to Touhou Project original content, original game files, or any other third-party materials.

The patch includes `th08-multi-LICENSE.txt`. When copying, modifying, or redistributing the source code, the applicable license and copyright notices must be retained.

This project is based on or references the following publicly available projects and designs:

- TH08 source reconstruction: https://github.com/N0zoM1z0/th08
- th06_multi_net: https://github.com/RUEEE/th06_multi_net
- Microsoft Detours: https://github.com/microsoft/Detours

Thanks to the authors of the original projects, historical contributors, and testers.

Special thanks to the following contributors for helping with development and testing:

- HDZsuper: https://github.com/HDZsuper
- nmmuwu: https://github.com/nmmuwu

Their assistance with testing and development contributed to the improvement and stability of this release. Each third-party project remains subject to its own respective license.

Parts of the development, debugging, and code review process were assisted by OpenAI Codex.

## 8. Bug Reports and Security Reports

General bugs may be reported through GitHub Issues. When reporting a crash, please include the patch version, stage, reproduction steps, and exception or error code.

Before uploading logs, screenshots, or crash dumps, remove private information such as public IP addresses, usernames, and personal file paths.

Do **not** upload `th08.exe`, DAT files, music, artwork, save data, or any other original game content.

Security vulnerabilities should not be publicly disclosed. If GitHub Private Vulnerability Reporting is enabled for the repository, please use that feature. Otherwise, contact the maintainer privately before disclosing any vulnerability details. Do not publish exploitation details in a public GitHub Issue.

This project is provided under the MIT License on an **“AS IS”** basis and makes no guarantee that it is free of bugs or crashes, or that it is suitable for any particular purpose.

## 9. Uninstallation

After closing the game, delete the following patch files:

- `th08-multi.exe`
- `th08-multi-launcher.exe`
- `th08_multi.ini`
- `th08_multi.ini.example`
- `th08-multi-README.txt`
- `th08-multi-LICENSE.txt`
- `联机教程.txt`
- `SHA256SUMS.txt`

The original game files will not be affected. If you wish to keep your multiplayer configuration, back up `th08_multi.ini` before uninstalling.
