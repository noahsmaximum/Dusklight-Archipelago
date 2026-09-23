## Install

1. **Mod** — put `archipelago.dusk` in your Dusklight mods folder:
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. **Apworld** — put `tp_dusklight.apworld` in your Archipelago `custom_worlds` folder.
3. **YAML** — grab a preset below, or generate a template from the Archipelago Launcher. Set `name:` to your slot name.

Then in Dusklight: use the arrows on the Play button to pick **Archipelago**, start a **new file**, and enter your server address, slot name and password. The seed builds from the server and you play.

Needs Dusklight 2.0.1 or newer.

## What's new in 0.4.2

- **Fixed the apworld manifest.** Archipelago 0.6.7 logs "Invalid or missing manifest file" for the old one and warns it stops working in 0.7.0. The manifest now carries the fields Archipelago expects.

Recent releases: death link in 0.4.0 (`death_link: true`, or the toggle in the **Archipelago** tab), and 0.4.1 refuses a seed whose apworld and mod disagree about the logic.

## Updating

Update the mod and the apworld **together** — the logic data and some location names changed, and the mod can't detect a mismatch. Finish any multiworld already in progress on the versions you started it with.
