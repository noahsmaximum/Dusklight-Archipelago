## Install

1. **Mod** — put `archipelago.dusk` in your Dusklight mods folder:
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. **Apworld** — put `tp_dusklight.apworld` in your Archipelago `custom_worlds` folder.
3. **YAML** — grab a preset below, or generate a template from the Archipelago Launcher. Set `name:` to your slot name.

Then in Dusklight: use the arrows on the Play button to pick **Archipelago**, start a **new file**, and enter your server address, slot name and password. The seed builds from the server and you play.

Needs Dusklight 2.0.1 or newer.

## What's new in 0.5.0

- **Seven presets, sized by checks.** Easy ~150, Normal ~200, Hard ~300, Extreme ~450, and three full games — Ultimate (Normal difficulty), Hero of Twilight (Hard) and Hero of Time (hardest, every junk item a trap) — at ~570.
- **New option: Shuffled Dungeons (0–9).** Only that many dungeons, picked at random, are checks; the rest keep their vanilla contents. You still play them, they just aren't checks. This is how the shorter presets get short.
- **Leaving an option out now gives the most checks.** Every optional shuffle is on by default, so the default template is the whole game. Start from a preset for anything shorter.
- **The template ships with every release** as `Template.yaml`, with every option explained in the randomizer's own words.
- **WebSocket compression.** Archipelago servers no longer warn that your client doesn't support compressed connections.

## Updating

Update the mod and the apworld **together**. The generating host needs the new apworld for the new presets and options; if the logic data ever differs between a player's mod and the seed, the mod refuses it and says to update. Finish any multiworld already in progress on the versions you started it with.
