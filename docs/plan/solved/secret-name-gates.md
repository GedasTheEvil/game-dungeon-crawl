# Secret name gates: rooms for those who read

Status: solved 2026-10-19 (play-tested by the user). Draft 2026-10-14, from the user. Builds on
[skill-hieroglyphs](skill-hieroglyphs.md) ("Words of Thoth") and the riddle gate's answer scroll
([riddles](../../riddles.md)).

## Why

Words of Thoth gives XP and lore today, little to play for, and nothing that changes from run to run. A teleporter
sealed by a name, the name written in a cartouche on the same level, makes it the key to secret rooms that only its
holders enter. Lore: Isis took power over Ra by learning his secret name; whoever knows the name opens the way.

## Idea

* **Name gate**: a teleporter that asks for a name before it works. Interact: the riddle scroll, theme "The sealed
  way", question "Speak the name written in the cartouche." The right name opens it for good (it then works as a
  normal teleporter, both ways). Wrong answers can be retried, no hint, no XP for the answer itself.
  A new gate type or a flag on `GateTeleport` (implementer's choice); its partner inside the room is a plain
  teleporter.
* **Unstable until named** (user, 2026-10-14): the name gate looks like a normal teleporter (columns, plasma), but
  its plasma pulses into transparency and back (a slow breath, ~1.5 s, alpha down to ~0.15, maybe a flicker) until
  the name is spoken. Then it is a normal, steady teleporter. The look says "this one is different" without a new
  model. Hook: `Dungeon::drawTeleporterTile` / `drawPortal` (`src/world/dungeon_render.cpp`), an alpha factor from
  the tile's state and the clock. A short shimmer from pulsing to steady on the right answer is a nice touch.
* **Name cartouche**: an inscription (style `cartouche`, the oval that holds a royal name: the authentic use) on
  the same level, reachable without the gate, not next to it. With the skill: the journal page says
  "The name of the king: KHUFU" plus a pencilled comment ("Somewhere on this level, something wants to hear it.").
  Without it: copied as glyphs like any other ("No idea what it says"); the name gate's refusal adds "A name is
  asked. The wall glyphs I copied might hold it."
* **Random per run**: each level's name is picked when a new game starts from the name list, no name twice in a
  run, and so is its cartouche picture (below). Both are saved (a seed or the per-level picks; save bump).
* **Pictures do not spell the names** (user, 2026-10-14): a fixed set of pre-baked name cartouches (about 16,
  plausible royal-looking sign groups), used only by name cartouches, any picture for any name. The game shows
  hieroglyphs, the skill "translates" them: nobody checks the spelling. It keeps names pure text (no bake per name)
  and leaves no way to decode a copy without the skill, so the room really is the skill's. No picture twice on a
  level; across levels repeats are fine.
* **Names are content** (user, 2026-10-14): `content/names/*.txt`, loaded at start-up like riddles and
  inscriptions, so names are added without a rebuild of the game. Format (document it in `docs/inscriptions.md` or a
  new `docs/names.md`):

  ```
  name: Ramesses
  A: ramesses
  A: ramses
  N: Ramesses the Great. Built half of Egypt, signed the other half.
  ```

  `name:` shown in the journal ("The name of the king: RAMESSES"), `A:` accepted spellings (compared like riddle
  answers), `N:` an optional pencilled comment. Adding a name: one entry, nothing to bake or build.
  The cartouche pictures: baked once by `tools/textures/inscriptions.py` (or a sibling tool) into their own cells
  (a names atlas `media/textures/decorations/name_cartouches.png`, or spare cells of the inscription atlas;
  implementer's choice), sign groups listed in the tool or a small content file.
  Start with about 30: Khufu, Djoser, Sneferu, Hatshepsut, Thutmose, Amenhotep, Akhenaten, Nefertiti,
  Tutankhamun, Ramesses, Seti, Merneptah, Narmer, Khafre, Menkaure, Unas, Teti, Pepi, Ahmose, Sobekneferu, Taharqa,
  Psamtik, Nectanebo, Cleopatra, Hor-Aha, Den, Mentuhotep, Senusret, Amenemhat, Horemheb. The list needs at least
  as many names as name gates in the campaign; the unit test checks it.
* **The secret room**: a pocket of the level grid only reached by the gate's teleporter. Holds a reward worth part
  of a heka: a chest with a rare potion or an amulet, sometimes a strong weapon roll, plus an inscription with lore
  (the room's story). Optional and off the exit path: no keys, levers or the exit inside. A guardian now and then
  (a mummy in a sarcophagus fits).
* **Placement**: levels 6-30 (the skill comes with the first heka, at lvl5), about one every second level
  (12-13 rooms). A player who learns the skill late misses the earlier rooms: the choice stays a choice.
* **Ties to other drafts**: the gate can look like a [ring transporter](../ring-transporter.md) platform;
  the [Stargate chamber](../stargate-chamber.draft.opus.md) can be one of the secret rooms (the name as its "address").

## Recommended choices (implementer may change)

* Typed answer, reusing the riddle scroll: it keeps the "password" feel and costs no new UI. Case and spaces ignored
  as for riddles; accept the common spellings (Ramesses / Ramses, Amenhotep / Amenophis).
* Not auto-opened for skill holders: they still read the cartouche first, so the level is explored, not skipped.
* Copies translated on learning the skill also reveal names, but only matter for the current level.

## Checker (`levelcheck`)

* The exit stays reachable with every name gate closed (walk the level with name gates treated as walls).
* Each name gate has a name cartouche on the same level, reachable with the gates closed; at most one name gate per
  level (one name per level keeps the rule simple).
* Nothing required inside: no key, lever or exit behind a name gate.
* Fix any level the new checks flag, as for every checker change.

## Tests

* Unit: the name pick is stable for a seed, differs between seeds, no repeat in a run, save / load keeps it;
  the gate refuses a wrong name, opens on the right one (spelling variants too) and stays open after a load;
  the checker rules above (a level with the exit behind a name gate warns).
* Scenario with screenshots: the pulsing plasma at two moments (faint and full), the steady plasma after the
  answer, the cartouche read (journal page with the name), the scroll at the gate, arrival in
  the room.
* Unit: the shipped name files load with no problems, every name has at least one `A:`; the picture pick is
  stable for a seed and never repeats on a level.
* Glossary rows: name gate, name cartouche, name list.

## Done

* The name gate is a teleporter with `NAME_SEAL` (256) on its pair id (`level.h`, glyph `!`): the save keeps the bit
  like any gate state, `Dungeon::SpeakName` takes it off. Its plasma breathes (`Dungeon::nameGateAlpha`, 1.5 s down to
  0.15 with a flicker) and shimmers 0.9 s when opened. Interact: Answer; the riddle scroll (`Riddle::AskName`): "The
  Name Gate", no reward, no hint; without the skill a wrong name adds the copied-glyphs line.
* The name cartouche is an inscription with the text id 100 (`NAME_CARTOUCHE_ID`, glyph `:`): read, copied, translated
  and given XP like one (no boon). Journal page "A royal name", "The name of the king: KHUFU" (a `title:` line gives
  queens theirs), the name's comment and "Something on this level wants to hear it." Journal save version 10 (the
  picture).
* `content/names/kings.txt`: the 30 names of the draft, with spellings ([names](../../names.md)). The pick: New Game
  rolls `RunStats::nameSeed` (a `NAMES` line after the run's, old saves seed 0), a shuffle per seed, level n the n-th
  name; the picture `nameCartouchePicture(seed, level)`. 16 pictures, `NAME_CARTOUCHES` in
  `tools/textures/inscriptions.py`, baked to `name_cartouches.png`.
* Checker: `checkNameGates` (warnings in [levels](../../levels.md)); `levelcheck` warns about a name gate before level 6.
* 13 rooms: levels 6, 8, 10, 12, 14, 16, 18, 21 (20 has no rock for a room), 22, 24, 26, 28, 30. Each a pocket in the
  rock with its teleporter, a lore inscription (ids 33 to 45; 38 gives Might +1, 45 max health +10%), one or two chests
  (rare potions, amulets up to grand strength and grand regeneration in 30), a mummy in 14, 21, 26 and 30. The small
  levels got two rows of rock on top for it.
* Tests: `tests/unit/names_test.cpp` (files, pick, save, gate, checker, campaign), the screenshot scenario
  `tests/scenarios/solved/secret_name_gate.txt` (on `tests/levels/name_gate`, with the scenario command `speakname`).
* Not done: the ring transporter look, the Stargate chamber as a room (their own drafts).
