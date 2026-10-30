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

Six gems, three metals: 18 item kinds. The gem says what it does, the metal how much (the amulet's tier, in short:
copper, silver, gold).

| gem | effect | copper | silver | gold |
|---|---|---|---|---|
| ruby (red) | regeneration, also in a fight | 1 HP / 8 s | 1 HP / 5 s | 1 HP / 3 s |
| sapphire (blue) | stamina comes back faster | +15% | +25% | +40% |
| emerald (green) | poison wears off faster | +20% | +35% | +50% |
| topaz (yellow) | walk and sprint faster | +5% | +8% | +12% |
| rock crystal (clear, a faint blue rim) | longer reach, melee and ranged | +10% | +15% | +25% |
| amethyst (purple) | cuts through the monster's resistances | 1/4 | 1/2 | all |

* Ruby vs. the regeneration amulet (Faience Lotus, 1 or 2 HP a second, only when safe): the ring heals far slower,
  but always, also with monsters chasing. Not while poisoned (as the amulet). Its own carry in
  `PlayerStats::Regenerate` (or a second function), not the amulet's `regen_carry_ms`.
* Sapphire: a factor on `PlayerStats::RegenerateStamina`'s rate. No amulet does stamina.
* Emerald: poison runs out sooner (`Poison::Advance`, its time left goes down faster); the poison amulet only
  stops it from starting. Its damage a tick stays.
* Topaz: nothing gives speed yet. Walk, sprint and climb on the ground only. **The jump carries as without the
  ring** (its forward drift and the walk in the air): the level checker and every gap in the campaign assume the
  jump's reach (`Jump::WALKING_REACH`, the `static_assert`s in `movement.h`), a longer jump would clear gaps that
  are meant to stop the player. The slowness curse still halves the (faster) walk.
* Rock crystal: a share more `weaponReach` (melee: from the box edge; ranged: how far it aims), stacks with the
  Sharp eye skill (`SHARP_EYE_RANGE_PERCENT`, multiplied). Small on purpose: a dagger reaches a bit farther, it does
  not become a spear. The minimum range of `ranged-weaker-than-melee` (if done by then) stays.
* Amethyst: a resistance (a damage type's multiplier under `NORMAL`, `damage.h`) moves that share of the way to
  `NORMAL`: `RESISTS` 50 becomes 62 / 75 / 100. Weaknesses (`WEAK`) stay as they are. Every type, fire too. Only
  the player's hits. The strongest ring: rarer than the others in the rolls (half their weight), gold amethyst only
  from about level 30.
* Numbers are a start: check them against the monster balance (`monster-balance` plan, the play-test levels).

Both rings always count, also two exactly the same (two gold rubies: 1 HP / 1.5 s; two copper topazes: +10%).
Rings are few in a playthrough, the player may well hold only two of the same. Caps: the amethyst at `NORMAL` (two silver = all),
the topaz at +20%, the crystal at +40%. Cap more if a stack turns out too strong in the play-test.

Left out (the user's ideas, why not):

* A longer (or higher) jump: the levels and the checker are built on the jump's reach and peak; a longer one breaks
  gaps and ledges all over the campaign. Not a ring.
* A yellow gem only for colour: the topaz has a job now (speed).

Names, as the amulets (the card's name by the object, the label by its effect):

* name: `Gold Ruby Ring`, short (slot name band): `Ruby III` (copper I, silver II, gold III), label:
  `gold ring of regeneration`, effect line: `Heals 1 health every 3 s`. The longest short names: `Amethyst III`,
  `Crystal III`; check they fit the band.
* Lore lines in `content/texts/items.txt` (`[item.<slug>]`), one per gem (or per ring).
* The Shen Ring amulet (stab guard) has "Ring" in its name; it stays an amulet. If it confuses next to the real rings,
  rename it (`Shen Amulet`?), implementer's call.

## Wearing two

* `ItemBag`: `std::array<std::optional<ItemKind>, 2> rings` (left, right) next to `worn`. A ring worn is one of
  those counted (as the amulet). Two copies of the same ring can be worn only if two are held.
* The ring's card (the user's choice): two buttons, `Left hand` and `Right hand`, where the others have `Use` and
  `Upgrade` (`Inventory::TwoButtons`, `inventory_layout.h`). A button puts the ring on that hand (the ring there
  comes off); on the hand it is worn on, the button reads `Take off`. Worn on the left, `Right hand` moves it over.
* The quick way (the Use key / double click on the slot, as an amulet): worn → taken off. Not worn → the free hand
  (left first); both full → it replaces the ring worn longest.
* The slot shows the hand: a small `L` / `R` mark where the amulet slot shows "worn".
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
* No ring placed by hand in the levels (no level file changes, `levelcheck` untouched). The first ring comes by
  chance, maybe late in the campaign: fine (the user), no early ring to teach them, no tutorial.

## Look

Two models, so the gem can blend apart from the band:

* `media/models/items/ring_band.md3`: the band with a raised setting, opaque. `ring_gem.md3`: the cut gem that sits
  in it, its own mesh and texture. Built in Blender (docs/modeling.md, a new script next to
  `tools/blender/models/amulets.py`), sized to read on the floor and in the inventory slot (bigger than real, like
  the amulets).
* Metal: copper (orange-brown), silver (cold grey-white), gold (as the amulets' gold). Three textures or a tint on
  one band texture.
* Gem: **semi-transparent** (the user's wish), tinted per gem on one gem model. The colours are the implementer's
  (the user left them): ruby deep red, sapphire blue, emerald green, topaz golden yellow (apart from the gold band:
  lighter, more lemon), rock crystal near clear with a cold blue rim, amethyst violet.
* The gem shines more than other items (the user's wish): a bright core / rim and a soft glint (a slow sparkle or
  a small additive glow, not a light that lights the room), on the floor, in a chest and in the slot. It reads in
  the dark and makes a ring easy to spot.
* The engine cannot do it yet: an `Item` is one model with one texture (`src/entities/item.h`), drawn opaque. Add an
  optional second part: a model, texture and tint drawn right after the first with the same transform, with alpha
  blending and no depth write (after the opaque scene where the item lies in the world; in the inventory slot right
  after its band). Same in every place an item draws: on the floor, in the chest, the inventory slot and card. Kept
  general, so a potion's glass or an amulet's stone can use it later.
* The 18 rings share the two models; metal and gem as texture or tint, so no 18 models.

## Notes and texts

* Journal: a field note the first time a ring of each gem is found, as the amulets' (`FieldNote::AmuletFirst` ..
  `AmuletLast`, plan amulet-extras): `RingFirst` .. `RingLast`, texts in `content/texts/`. The first ring's note also
  says a ring goes on each hand.
* The player's stats (wherever the worn amulet's bonus shows: the stats card, the HUD tooltip) show the rings' sum.

## Tests

* Unit (`tests/unit/`): the ring bonus sums (two rings, same gem stacks, two identical rings stack, the caps),
  left / right / take off / move to the other hand, the quick way (free hand, then the oldest), the ruby heals in a fight and
  not while poisoned, the emerald shortens poison, sapphire's stamina rate, the topaz's walk (the jump's reach the
  same), the crystal's reach (with Sharp eye), the amethyst's resistance (weakness untouched), the save round trip
  with two worn rings, the loot rolls (metal by level, the amethyst rarer).
* Scenario: inventory screenshot of the Rings tab with two worn rings (L / R marks, name band width of the longest
  short name), a ring on the floor and in a chest (gem see-through).
* Play script `tests/play/ring-accessories.txt`: `give` a few rings, a monster to fight.
* Glossary: rows for ring, the six gems, the metal; the `group` row loses "no rings yet".

## Open points (implementer's choice, warn when reporting)

* The effects are not final (user). Ruby, topaz (speed), crystal (reach) and amethyst (resistances) are the
  user's ideas; sapphire and emerald proposals. Spare ring-only ideas: longer potion effects, curses wear off
  faster, more light radius, a chance to find one bonus item more in chests.
* Which gem has which effect: the colours are a guess (yellow = speed, clear = sight / reach, purple = the rare,
  strong one).
* Upgrades later (three copper → one silver?). Not in this plan.
* A ring seen on the player's fist (`Item::DrawHeld`, the first-person hand): nice, not in this plan.

## Implemented (2026-10-30)

Choices made where the plan left them open:

* The effects and numbers as in the table; not yet checked against the balance sim (`monster-balance`).
* Labels (scenario slugs, editor words): ring of regeneration, vigor, cleansing, speed, reach, sundering
  (`gold_ring_of_regeneration`). The rock crystal is "Crystal" in names ("Silver Crystal Ring", "Crystal II").
* Drops: mummy and Anubis statue 6% of kills (`MonsterKind::ringDropPercent`), the cursed mimic's chest 15% (after
  its amulet roll). No boss ring: bosses keep their amulet only. Rings in chests: 8%; silver from level 11 at 40%,
  gold from level 30 at 15%; the amethyst at half the weight.
* The Shen Ring amulet keeps its name.
* The rings' sum shows on the ring card ("Worn: heals 1 health every 1.5 s"), not on the stats rows (no room) and
  the HUD has no tooltip.
* The topaz counts as airborne while jumping or falling (no faster walk in the air).
* On a chest a ring has its own size (`RING_CHEST_SCALE`), as the potions, not the weapons' minimum.
* The second part (`Item::loadPart`, `DrawPart`) is drawn unlit (emissive) with a slow additive glint; in the world
  after the opaque scene (`Dungeon::drawItemParts`).
* Models: `tools/blender/models/rings.py` (the amulets are in `items.py`, there is no `amulets.py`).

Tests: `tests/unit/ring_accessories_test.cpp`, `tests/scenarios/ring_accessories.txt` (level
`tests/levels/ring_accessories`), play script `tests/play/ring-accessories.txt`.
