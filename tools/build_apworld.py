"""Vendors the randomizer data into the apworld and packages tp_dusklight.apworld.

usage: python tools/build_apworld.py [--install <Archipelago>/custom_worlds] [--out <file.apworld>]
"""
from __future__ import annotations

import argparse
import json
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "apworld" / "tp_dusklight"
DATA = ROOT / "generator" / "data"
FILES = ["items.yaml", "locations.yaml", "macros.yaml", "settings_list.yaml"]


def sync_data() -> None:
    dst = SRC / "data"
    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True)
    for f in FILES:
        shutil.copyfile(DATA / f, dst / f)
    shutil.copytree(DATA / "world", dst / "world")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--install", type=Path,
                    help="copy the built .apworld into this dir (Archipelago's custom_worlds)")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "tp_dusklight.apworld")
    args = ap.parse_args()

    sync_data()
    manifest = json.loads((SRC / "archipelago.json").read_text(encoding="utf-8"))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("archipelago.json", json.dumps(manifest, indent=2))
        for path in sorted(SRC.rglob("*")):
            if path.is_dir() or "__pycache__" in path.parts:
                continue
            z.write(path, Path("tp_dusklight") / path.relative_to(SRC))
    print(f"wrote {args.out}")

    if args.install:
        # custom_worlds only loads .apworld files: an unzipped folder there fails to import,
        # and Archipelago then quietly uses any older copy it finds under worlds/ instead.
        stale = args.install / "tp_dusklight"
        if stale.is_dir():
            shutil.rmtree(stale)
        target = args.install / "tp_dusklight.apworld"
        shutil.copyfile(args.out, target)
        print(f"installed {target}")
        shadow = args.install.parent / "worlds" / "tp_dusklight"
        if shadow.exists():
            print(f"WARNING: {shadow} exists and will be loaded instead. Remove it.")


if __name__ == "__main__":
    main()
