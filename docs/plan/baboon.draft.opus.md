# Baboon of the Lake of Fire

Status: draft 2026-10-21, from the user. Needs first: [monster-kiting](monster-kiting.draft.opus.md) (it keeps its
distance and shoots). Its boss: [babi-boss](babi-boss.draft.opus.md).

## Lore

The Book of the Dead (chapter 126) and the Amduat put a lake of fire at the gate of the Duat, guarded by four baboons
that sit at its corners. Baboons were sacred to Thoth, and they greet the rising sun with raised hands. Babi, the
"bull of the baboons", eats the hearts of the unworthy by that lake. So the Lake of Fire zone
([campaign-zones](campaign-zones.draft.opus.md#zones-decided-2026-10-21)) is the baboons' home.

## Idea

* **Kind:** a quick walker and climber (`climbs`, ladders), mid HP, a ranged fighter first.
* **Ranged (preferred):** hurls a burning coal from the lake (`RangedRules` with fire in its `mix`, so it sets the
  player burning, drawn as a fireball like the Anubis's bolt). Range about 4-5 tiles, a cooldown of 2-3 s. Water puts
  the burning out, and [fire-in-water](fire-in-water.draft.sonet.md) cuts the fire share of a hit to a tenth in water:
  the lake zone's pools are the player's cover.
* **Melee (fallback):** a bite with long fangs (slash, pierce), when cornered or the player is in reach.
* **AI:** a kiter ([monster-kiting](monster-kiting.draft.opus.md)): keeps the player at its range, backs off when they
  come at it, never far enough to lose them.
* **Tell:** a screech before each throw (its ranged clip lifts the coal over its head first), so the player can dodge
  by a jump or by stepping off the row.
* **Resistances:** resists fire (it lives by the lake), weak to pierce (arrows, javelins), normal otherwise.
* **Journal note**, in the archaeologist's voice: "Thoth's baboons, the priests said. Thoth was a scribe. These
  throw things."
* **Levels:** the Lake of Fire zone, three to five a level; a few in the Necropolis after it. levelgen: the zone's
  monster set ([campaign-levelgen](campaign-levelgen.draft.opus.md)); until then, from difficulty 9.
* **Glyph:** pick a free one. **Model:** procedural ([modeling.md](../modeling.md)): a hamadryas baboon, mane on the
  shoulders, a long dog-like muzzle, a crouching walk on all fours. Clips: idle (sitting, hands raised), walk, climb,
  bite, ranged (throw), die. Sounds: screech, bark, throw, die (`tools/audio/`).

## Tests

* Unit (sim): its shot sets the player burning; in water the hit deals a tenth of its fire share (if fire-in-water is
  in); the kiting tests of monster-kiting with the baboon.
* Scenario with screenshots: the throw, the fireball, the bite.
* `bin/levelcheck` with no warnings on the levels that place it.
