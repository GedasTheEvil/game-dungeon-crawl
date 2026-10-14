# Secret name gates: rooms for those who read

Status: draft 2026-10-14, ready (needs nothing first). From the user. Builds on
[skill-hieroglyphs](solved/skill-hieroglyphs.md) ("Words of Thoth") and the riddle gate's answer scroll
([riddles](../riddles.md)).

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
* **Name cartouche**: an inscription (style `cartouche`, the oval that holds a royal name: the authentic use) on
  the same level, reachable without the gate, not next to it. With the skill: the journal page says
  "The name of the king: KHUFU" plus a pencilled comment ("Somewhere on this level, something wants to hear it.").
  Without it: copied as glyphs like any other ("No idea what it says"); the name gate's refusal adds "A name is
  asked. The wall glyphs I copied might hold it."
* **Random per run**: each level's name is picked when a new game starts, from a list of about 24 royal and divine
  names (Khufu, Djoser, Sneferu, Hatshepsut, Thutmose, Amenhotep, Akhenaten, Nefertiti, Tutankhamun, Ramesses,
  Seti, Merneptah, Narmer, Khafre, Menkaure, Unas, Teti, Pepi, Ahmose, Sobekneferu, Taharqa, Psamtik, Nectanebo,
  Cleopatra). Each name has its real cartouche spelling pre-baked in the inscription atlas (one cell per name), so the
  atlas pipeline stays as is. No name twice in a run. The pick is saved (a seed or the per-level names; save bump).
  A player who reads real hieroglyphs can decode the copy without the skill: an easter egg, fine.
* **The secret room**: a pocket of the level grid only reached by the gate's teleporter. Holds a reward worth part
  of a heka: a chest with a rare potion or an amulet, sometimes a strong weapon roll, plus an inscription with lore
  (the room's story). Optional and off the exit path: no keys, levers or the exit inside. A guardian now and then
  (a mummy in a sarcophagus fits).
* **Placement**: levels 6-30 (the skill comes with the first heka, at lvl5), about one every second level
  (12-13 rooms). A player who learns the skill late misses the earlier rooms: the choice stays a choice.
* **Ties to other drafts**: the gate can look like a [ring transporter](ring-transporter.draft.opus.md) platform;
  the [Stargate chamber](stargate-chamber.draft.opus.md) can be one of the secret rooms (the name as its "address").

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
* Scenario with screenshots: the cartouche read (journal page with the name), the scroll at the gate, arrival in
  the room.
* Glossary rows: name gate, name cartouche.
