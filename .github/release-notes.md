## Install

1. **Mod** — put `archipelago.dusk` in your Dusklight mods folder:
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. **Apworld** — put `tp_dusklight.apworld` in your Archipelago `custom_worlds` folder.
3. **YAML** — grab a preset below, or generate a template from the Archipelago Launcher. Set `name:` to your slot name.

Then in Dusklight: use the arrows on the Play button to pick **Archipelago**, start a **new file**, and enter your server address, slot name and password. The seed builds from the server and you play.

Needs Dusklight 2.0.1 or newer.

## What's new in 0.5.1

- **Hyrule Castle no longer holds other players' important items.** It's the final dungeon, so anything a friend needed from there only turned up at the very end of your game. Every preset and the template exclude it, and it's now the default. Each dungeon is a location group: `exclude_locations: [Hyrule Castle, Palace of Twilight]` works too, and `[]` turns it off.
- **Easy, Normal and Hard raise progression balancing** (90, 80, 70), so this game's progression turns up earlier and there's less waiting in the long tail.

New in 0.5.0: seven presets sized by checks, the Shuffled Dungeons option, and WebSocket compression.

## Updating

Update the mod and the apworld **together**. The generating host needs the new apworld for the new presets and options; if the logic data ever differs between a player's mod and the seed, the mod refuses it and says to update. Finish any multiworld already in progress on the versions you started it with.
