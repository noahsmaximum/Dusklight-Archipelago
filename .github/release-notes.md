## Install

1. **Mod** — put `archipelago.dusk` in your Dusklight mods folder:
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. **Apworld** — put `tp_dusklight.apworld` in your Archipelago `custom_worlds` folder.
3. **YAML** — grab a preset below, or generate a template from the Archipelago Launcher. Set `name:` to your slot name.
4. **Dusklight setting** — for Easy, Normal, Hard or Extreme, turn on **Can Transform Anywhere** (Settings → Cheats). Logic may expect you to transform where NPCs can see you.

Then in Dusklight: use the arrows on the Play button to pick **Archipelago**, start a **new file**, and enter your server address, slot name and password. The seed builds from the server and you play.

Needs Dusklight 2.0.1 or newer.

## What's new in 0.5.2

- **Easy, Normal, Hard and Extreme start with the Shadow Crystal**, so you can turn into a wolf from the beginning, and expect Transform Anywhere: turn on Dusklight's Can Transform Anywhere cheat for them (see Install). Ultimate, Hero of Twilight and Hero of Time keep both vanilla.
- **Starting inventory works.** `start_inventory` and `start_inventory_from_pool` used to stop generation with an error; they work now.

New in 0.5.1: Hyrule Castle holds no other player's important items by default, and Easy, Normal and Hard raise progression balancing.

## Updating

Update the mod and the apworld **together**. The generating host needs the new apworld for the new presets and options; if the logic data ever differs between a player's mod and the seed, the mod refuses it and says to update. Finish any multiworld already in progress on the versions you started it with.
