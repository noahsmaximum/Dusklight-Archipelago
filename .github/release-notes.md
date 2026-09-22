## Install

1. **Mod** — put `archipelago.dusk` in your Dusklight mods folder:
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. **Apworld** — put `tp_dusklight.apworld` in your Archipelago `custom_worlds` folder.
3. **YAML** — grab a preset below, or generate a template from the Archipelago Launcher. Set `name:` to your slot name.

Then in Dusklight: use the arrows on the Play button to pick **Archipelago**, start a **new file**, and enter your server address, slot name and password. The seed builds from the server and you play.

Needs Dusklight 2.0.1 or newer.

## What's new in 0.3.0

- **Randomizer fixes from upstream**, including a logic change: the two checks at the Faron Woods Owl Statue now also need a way to smash.
- Fixed returning to spawn part-way through the sewers sequence, the Midna jump to Coro's house outside twilight, and deleting a seed that's actively in use on a file.
- Removed the unintuitive lost-woods-ledge savewarp shortcut.
- Renamed a location, a macro and a setting from the "Canon"/"Canonball" typo to the correct "Cannon"/"Cannonball" spelling.
- Spoiler logs are now valid YAML.

## Updating

Update the mod and the apworld **together** — the logic data and some location names changed, and the mod can't detect a mismatch. Finish any multiworld already in progress on the versions you started it with.
