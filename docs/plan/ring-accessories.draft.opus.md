# Rings: two worn at once, one per hand

Status: draft 2026-10-30 (user). Needs nothing first. Open points at the end (ring effects not final, upgrades
later).

## Idea

The inventory has a Rings tab (`ItemGroup::Rings`, `GROUP_NAMES` in `src/ui/inventory.cpp`) but no rings: the tab
stays disabled. Add them.

* Unlike a weapon or an amulet (one at a time), the player wears **two rings, one per hand**.
* A ring is weaker than an amulet, or gives what no amulet gives. Not a smaller copy of an amulet.
* No upgrades yet (no spares, no points, `CanUpgrade` says no).

## The rings: gem = effect, metal = strength

Three gems, three metals: 9 item kinds. The gem says what it does, the metal how much (the amulet's tier, in short:
copper, silver, gold).

| gem | effect | copper | silver | gold |
|---|---|---|---|---|
| ruby (red) | regeneration, also in a fight | 1 HP / 8 s | 1 HP / 5 s | 1 HP / 3 s |
| sapphire (blue) | stamina comes back faster | +15% | +25% | +40% |
| emerald (green) | poison wears off faster | +20% | +35% | +50% |

* Ruby vs. the regeneration amulet (Faience Lotus, 1 or 2 HP a second, only when safe): the ring heals far slower,
  but always, also with monsters chasing. Not while poisoned (as the amulet). Its own carry in
  `PlayerStats::Regenerate` (or a second function), not the amulet's `regen_carry_ms`.
* Sapphire: a factor on `PlayerStats::RegenerateStamina`'s rate. No amulet does stamina.
* Emerald: poison runs out sooner (`Poison::Advance`, its time left goes down faster); the poison amulet only
  stops it from starting. Its damage a tick stays.
* Numbers are a start: check them against the monster balance (`monster-balance` plan, the play-test levels).

Two rings of the same gem stack (two gold rubies: 1 HP / 1.5 s). With three gems, "only the stronger counts" would
make the choice trivial; stacking makes "two rubies or ruby + sapphire" a real one. Cap if a stack turns out too
strong in the play-test.

Names, as the amulets (the card's name by the object, the label by its effect):

* name: `Gold Ruby Ring`, short (slot name band): `Ruby III` (copper I, silver II, gold III), label:
  `gold ring of regeneration`, effect line: `Heals 1 health every 3 s`.
* Lore lines in `content/texts/items.txt` (`[item.<slug>]`), one per gem (or per ring).
* The Shen Ring amulet (stab guard) has "Ring" in its name; it stays an amulet. If it confuses next to the real rings,
  rename it (`Shen Amulet`?), implementer's call.

## Wearing two

* `ItemBag`: `std::array<std::optional<ItemKind>, 2> rings` (left, right) next to `worn`. A ring worn is one of
  those counted (as the amulet). Two copies of the same ring can be worn only if two are held.
* Use on a ring in the Rings tab: worn → taken off. Not worn → put on the free hand (left first); both full → it
  replaces the ring worn longest. Show which hand in the slot (a small `L` / `R` mark where the amulet slot shows
  "worn").
* `PlayerStats` gets a `RingBonus` (as `AmuletBonus`), the sum of both rings, applied when a ring goes on or off.
* HUD: `PlayerHud::Icon::Ring` already exists in the atlas (`hud_icons.py`): show the worn rings like the worn
  amulet, two small gem dots or two icons.
* Save: a new inventory version (`INV<n+1>`) with the two worn ring kinds; older saves load with none. Item kinds
  appended at the end of `ItemKind` so the save ids of the rest stay (`item_bag.h` notes).
* Sound: reuse `amulet_s` (put on / taken off) unless a ring clink is easy.

## Where they come from

Metal by depth, like the amulet tiers (`loot.cpp`):

* Chests: `RollChestLoot` gets a `CHEST_RING_CHANCE` (about 8%, rolled after the amulet, so the old rolls stay the
  same where it matters). Copper from the start, silver from level ~11 (`MINOR_AMULET_LEVEL`), gold from ~level 30
  and rare. Trapped chests and the cursed mimic's chest pick it up through `RollChestLoot`.
* Monster drops: `RollKillDrop` today gives a weapon copy (`KILL_DROP_CHANCE` 5%) or the boss's amulet. Some kinds
  drop a ring instead, now and then: the ones that "carry" things (mummy, mummy priest, the cursed mimic, Anubis
  statue; the implementer picks, a `ringDrop` percent in `MonsterKind`). Bosses keep their amulet; a boss may add a
  silver or gold ring (implementer's call).
* No ring placed by hand in the levels for now (no level file changes, `levelcheck` untouched), unless the level
  format already takes any item kind (then a few in the campaign are fine).

## Look

* One model, `media/models/items/ring.md3`, built in Blender (docs/modeling.md, a new script next to
  `tools/blender/models/amulets.py`): a band with a raised setting and a cut gem, sized to read on the floor and in
  the inventory slot (bigger than real, like the amulets).
* Metal: copper (orange-brown), silver (cold grey-white), gold (as the amulets' gold). Three textures or a tint on
  one band texture.
* Gem: ruby, sapphire, emerald, **semi-transparent** (the user's wish): drawn after the opaque items with alpha
  blending, a bright core / rim so it still reads as a gem in the dark. Check how the item renderer draws (the potion
  liquids may already blend); if it cannot, add that path.
* The 9 rings share the model; metal and gem as material or tint, so no 9 models.

## Tests

* Unit (`tests/unit/`): the ring bonus sums (two rings, same gem stacks), the ruby heals in a fight and not while
  poisoned, the emerald shortens poison, sapphire's stamina rate, the save round trip with two worn rings, the loot
  rolls (metal by level).
* Scenario: inventory screenshot of the Rings tab with two worn rings (L / R marks, name band width of the longest
  short name), a ring on the floor and in a chest (gem see-through).
* Play script `tests/play/ring-accessories.txt`: `give` a few rings, a monster to fight.
* Glossary: rows for ring, the three gems, the metal; the `group` row loses "no rings yet".

## Open points (implementer's choice, warn when reporting)

* The effects are not final (user). Ruby = regeneration is the user's; sapphire and emerald are proposals. Other
  ring-only ideas: longer potion effects, faster sprint, curses wear off faster, more light radius, a chance to find
  one bonus item more in chests.
* Upgrades later (three copper → one silver?). Not in this plan.
* Both hands full: replace the oldest, or let the player pick the hand (a small choice on Use).
