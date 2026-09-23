"""Regenerate the player YAML template shipped with every release.

    python tools/make_template.py <Archipelago checkout> [--python <interpreter>]

Builds the apworld, installs it into that Archipelago's custom_worlds, and has Archipelago's
own template generator render it in a fresh process (a running Launcher keeps whatever
apworld it loaded at startup, so templates made from one can be stale). Writes
presets/Template.yaml, which CI attaches to releases alongside the presets.

Archipelago stamps `requires: version:` with its own version, and refuses a YAML that asks
for a newer one than it's running, so that line is rewritten to the apworld's
minimum_ap_version: a template rendered on 0.6.8 would otherwise lock out 0.6.7 players.
Re-run this whenever the options change.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAME = "Twilight Princess (Dusklight)"
# Not the Launcher's "Twilight Princess (Dusklight).yaml": GitHub rewrites spaces in release
# asset names, so the download wouldn't match what the README tells people to look for.
OUT = ROOT / "presets" / "Template.yaml"

RENDER = """
import sys
sys.path.insert(0, {ap!r})
import worlds
from Options import generate_yaml_templates
generate_yaml_templates({out!r}, False)
"""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("archipelago", type=Path)
    ap.add_argument("--python", default=sys.executable)
    ap.add_argument("--no-install", action="store_true",
                    help="use the apworld already installed there (e.g. while generations run)")
    args = ap.parse_args()

    if not args.no_install:
        subprocess.run([sys.executable, str(ROOT / "tools" / "build_apworld.py"),
                        "--install", str(args.archipelago / "custom_worlds")], check=True)

    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run([args.python, "-c", RENDER.format(ap=str(args.archipelago), out=tmp)],
                       cwd=args.archipelago, check=True,
                       env={**__import__("os").environ, "SKIP_REQUIREMENTS_UPDATE": "1"})
        text = (Path(tmp) / f"{GAME}.yaml").read_text(encoding="utf-8-sig")

    manifest = json.loads((ROOT / "apworld" / "tp_dusklight" / "archipelago.json")
                          .read_text(encoding="utf-8"))
    minimum = manifest["minimum_ap_version"]
    text, n = re.subn(r"(?m)^(  version: )\S+( # Version of Archipelago required)",
                      rf"\g<1>{minimum}\g<2>", text)
    if n != 1:
        raise SystemExit("could not find the requires: version: line to rewrite")
    OUT.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {OUT} (requires Archipelago {minimum}, world {manifest['world_version']})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
