#!/usr/bin/env python3
"""Create and optionally install the data-free th08-multi v0.22 patch."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT.parent / "th08-multi-v0.22-patch.zip"
EXPECTED_TH08_SIZE = 840_704
EXPECTED_TH08_SHA256 = "330fbdbf58a710829d65277b4f312cfbb38d5448b3df523e79350b879213d924"
ZIP_TIMESTAMP = (2026, 9, 11, 0, 0, 0)

PAYLOAD_SOURCES = {
    "th08-multi.exe": ROOT / "build" / "th08-multi.exe",
    "th08-multi-launcher.exe": ROOT / "build" / "th08-multi-launcher.exe",
    "th08_multi.ini.example": ROOT / "th08_multi.ini.example",
    "th08-multi-README.txt": ROOT / "docs" / "MULTIPLAYER_PATCH_README.txt",
    "th08-multi-LICENSE.txt": ROOT / "LICENSE",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verify_original_game(directory: Path) -> None:
    executable = directory / "th08.exe"
    if not executable.is_file():
        raise RuntimeError(f"original th08.exe is missing from {directory}")
    data = executable.read_bytes()
    if len(data) != EXPECTED_TH08_SIZE or sha256(data) != EXPECTED_TH08_SHA256:
        raise RuntimeError("th08.exe is not the supported Japanese TH08 1.00d executable")
    for name in ("th08.dat", "thbgm.dat"):
        if not (directory / name).is_file():
            raise RuntimeError(f"required original game data is missing: {name}")


def load_payload() -> dict[str, bytes]:
    payload: dict[str, bytes] = {}
    for name, source in PAYLOAD_SOURCES.items():
        if not source.is_file():
            raise RuntimeError(f"patch input is missing: {source}")
        payload[name] = source.read_bytes()
    checksums = "".join(
        f"{sha256(payload[name])}  {name}\n" for name in sorted(payload)
    )
    payload["SHA256SUMS.txt"] = checksums.encode("ascii")
    forbidden = {"th08.exe", "th08.dat", "thbgm.dat", "score.dat"}
    if forbidden.intersection(payload):
        raise RuntimeError("patch payload contains copyrighted or user-owned game files")
    return payload


def write_zip(output: Path, payload: dict[str, bytes]) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    with zipfile.ZipFile(
        temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as archive:
        for name in sorted(payload):
            info = zipfile.ZipInfo(name, ZIP_TIMESTAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, payload[name])
    temporary.replace(output)


def install_payload(directory: Path, payload: dict[str, bytes]) -> None:
    verify_original_game(directory)
    for name, data in payload.items():
        destination = directory / name
        temporary = directory / (name + ".tmp")
        temporary.write_bytes(data)
        temporary.replace(destination)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--install-dir",
        type=Path,
        help="also install the patch into a verified Japanese TH08 1.00d directory",
    )
    args = parser.parse_args()

    try:
        payload = load_payload()
        output = args.output.resolve()
        write_zip(output, payload)
        print(f"Packaged {output}")
        print(f"SHA-256 {sha256(output.read_bytes())}")
        if args.install_dir is not None:
            install_directory = args.install_dir.resolve()
            install_payload(install_directory, payload)
            print(f"Installed patch into {install_directory}")
    except (OSError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
