#!/usr/bin/env python3
"""Build and run the multiplayer packet parser under libFuzzer and sanitizers."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=60)
    args = parser.parse_args()
    if args.seconds < 1 or args.seconds > 3600:
        parser.error("--seconds must be between 1 and 3600")

    compiler = shutil.which("clang++")
    if compiler is None:
        print("error: clang++ with libFuzzer support is required", file=sys.stderr)
        return 2

    output_dir = ROOT / "build" / "security"
    output_dir.mkdir(parents=True, exist_ok=True)
    executable = output_dir / "multiplayer-protocol-fuzz"
    corpus = output_dir / "corpus"
    corpus.mkdir(exist_ok=True)
    command = [
        compiler,
        "-std=c++17",
        "-O1",
        "-g",
        "-fno-omit-frame-pointer",
        "-fsanitize=fuzzer,address,undefined",
        "-DTH08_MULTI_PROTOCOL_STANDALONE",
        "-Isrc",
        "tests/fuzz/MultiNetProtocolFuzz.cpp",
        "src/MultiNetProtocol.cpp",
        "-o",
        str(executable),
    ]
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run(
        [
            str(executable),
            str(corpus),
            f"-max_total_time={args.seconds}",
            "-timeout=5",
            "-rss_limit_mb=512",
            "-dict=tests/fuzz/MultiNetProtocol.dict",
        ],
        cwd=ROOT,
        check=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
