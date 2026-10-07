# More Egyptian weapons

Status: implemented 2026-10-07, not play tested (see [Done](#done)). Draft 2026-10-06, refined 2026-10-07. Depends on [inventory-overhaul.md](solved/inventory-overhaul.md): the
weapons group passes the 8 slots of one tab page. Comes after [attack-timers.md](attack-timers.md): frame
delay and recovery per weapon, hotkeys per class.

## Idea

Ancient Egypt had many weapons. Put all of them from
[screenshots/Egyptian-Weapons-Summary-Version-2.jpg](screenshots/Egyptian-Weapons-Summary-Version-2.jpg) into the game,
except the war chariot. 13 weapons, 9 of them new: the weapons group fills two tab pages.

## Decisions

* The sword becomes the short sword and the bow the self-bow: same `ItemKind`, same file ids, new names, lore and
  models. Saves and levels keep working.
* The club stays, as the wooden starter. The mace is the bronze blunt weapon for later.
* Damage types stay blunt, slash, pierce. Each weapon's role comes from its mix, damage, reach, speed and how it
  fires, not from new types.
* Javelin, throwing stick and sling stones are endless, like arrows: no ammo, no pick-up. The throwing stick flies
  back to the hand (a look only, it changes nothing).
* Hotkeys cycle per class, `1` melee, `2` ranged ([attack-timers.md](attack-timers.md)).
* New `ItemKind` values go in inventory order inside the weapons group; file ids (`ItemType`, `src/world/items.h`)
  only get new ids at the end of their type, so old saves and levels still read.

## Roles

Today's numbers (`ITEM_DEFS`, `src/state/assets.cpp`) for the kept weapons; the new ones are starting points beside
them, tuned with [monster-balance.draft.md](monster-balance.draft.md).

| Weapon | Status | Class | Mix blunt/slash/pierce | Damage | Reach (tenths) | Attack ms | Role |
|---|---|---|---|---|---|---|---|
| Club | kept | melee | 85/15/0 | 10 | 2 | 900 | wooden starter, blunt |
| Dagger | new | melee | 0/30/70 | 8 | 1 | 400 | fastest, shortest |
| Short sword | was sword | melee | 0/85/15 | 35 | 3 | 550 | quick all-rounder |
| Khopesh | new | melee | 0/100/0 | 45 | 3 | 700 | pure slash, hooked swing |
| Epsilon axe | new | melee | 30/70/0 | 55 | 3 | 950 | heavy slash |
| Duckbill axe | new | melee | 20/0/80 | 50 | 3 | 900 | pierces shells (scarabs) |
| Mace | new | melee | 100/0/0 | 50 | 2 | 1000 | heavy blunt (bats, mimic, Anubis) |
| Spear | kept | melee | 0/15/85 | 20 | 5 | 750 | longest melee, thrust |
| Self-bow | was bow | ranged | 0/0/100 | 12 | 30 | 1000 | long range |
| Composite bow | new | ranged | 0/0/100 | 22 | 40 | 1300 | longer, harder, slower draw |
| Sling | new | ranged | 100/0/0 | 10 | 20 | 800 | ranged blunt, flat flight |
| Throwing stick | new | ranged | 90/10/0 | 14 | 12 | 700 | short ranged blunt, spins, comes back |
| Javelin | new | ranged | 0/10/90 | 30 | 15 | 1200 | short, hard pierce throw |

### Attack timers

Frame delay and recovery per weapon, as defined in [attack-timers.md](attack-timers.md). Attack ms in the
table above is their sum. Melee: the dagger is quick on both, the spear thrusts fast and recovers long, the khopesh
flows from swing to swing, the axes and the mace wind up long. Club, short sword, spear and self-bow keep their values
from that plan.

| Weapon | Class | Frame delay ms | Recovery ms |
|---|---|---|---|
| Dagger | melee | 150 | 250 |
| Khopesh | melee | 300 | 400 |
| Epsilon axe | melee | 550 | 400 |
| Duckbill axe | melee | 500 | 400 |
| Mace | melee | 600 | 400 |
| Composite bow | ranged | 650 | 650 |
| Sling | ranged | 350 | 450 |
| Throwing stick | ranged | 300 | 400 |
| Javelin | ranged | 500 | 700 |

Thrown weapons and sling stones reuse the arrow flight (`Dungeon::ShootArrow`, `src/world/dungeon_arrows.cpp`) with
their own model, arc and wind-up motion.

## Campaign placement

Weapons come in by depth, so the player keeps finding something new over the 30 levels
([longer-campaign.md](longer-campaign.md)):

* Levels 1-5 (hand-made): club, dagger, short sword, self-bow, sling.
* Middle: spear, khopesh, throwing stick, javelin, epsilon axe.
* Deep: mace, duckbill axe, composite bow.

The levelcheck threat (`monster_kinds.cpp`) must still find a working weapon for each level's monsters.

## Stages

Each stage is committed and tested on its own.

1. **Renames**: sword to short sword, bow to self-bow: names, lore, models. Same ids.
2. **Ranged family**: sling, throwing stick, javelin, composite bow. The arrow flight becomes a general projectile
   (model, arc, spin, the stick's return look). The weapons tab gets its second page here.
3. **Melee family**: dagger, khopesh, epsilon axe, duckbill axe, mace.
4. **Campaign**: chest placement by depth, levelcheck on all levels, balance pass with
   [monster-balance.draft.md](monster-balance.draft.md).

Per new weapon: a Blender model ([../remodeling.md](../remodeling.md)), a `WEAPON_DEFS` row (mix, damage, reach,
motion, sounds), `ItemKind` and `ItemText` entries, a file id, sounds.

## Open

* Stamina cost per attack: none today; decide with [monster-balance.draft.md](monster-balance.draft.md).
* ~~The hand-made levels 1-5: which existing weapon chests swap to dagger or sling.~~ Lvl2 dagger, lvl3 sling
  (see Done).

## Done

Four commits, one per stage.

1. **Tables and renames.** A weapon is a row: `WEAPON_DEFS` (`src/state/assets.cpp`, `ItemKind` order) for model,
   damage, reach, mix, motion and sounds; `isRanged` from the order (melee first, then ranged); HUD icons from cell 8
   of the atlas on (`PlayerHud::weaponIcon`); loot grades in `loot.cpp`. Sword and bow became the short sword and the
   self-bow: names and lore changed, the models were already a bronze short sword and an acacia self-bow, so they
   stay. A save from before still reads (`OLD_SLOTS` in `item_bag.cpp`).
2. **Ranged family.** The arrow flight became a general missile (`Dungeon::Shoot`, `MISSILE_RULES` in
   `dungeon_arrows.cpp`): arrow, sling stone (flat arc), throwing stick (spins, flies back to the hand after a hit or a
   wall) and javelin (sticks like an arrow). A thrown weapon leaves the hand empty until the swing ends.
3. **Melee family.** Dagger, khopesh, epsilon axe, duckbill axe, mace.
4. **Campaign.** Chests swapped by depth (`tools/level/campaign/`, all 30 levels pass `./levelcheck` with no
   warnings); `levelgen` unlocks weapon kinds with the difficulty.

Per weapon: a model in `tools/blender/models/items.py`, a HUD icon in `tools/textures/hud_icons.py`, sounds from
`tools/audio/weapon_sounds.py` (new: sling whirl and release, throw, stone hit and wall, axe hit, mace hit; the others
reuse the club, sword, spear and bow sounds).

Decided on the way (the user was away; easy to change):

* **Placement.** The short sword (first at lvl11) and the self-bow (lvl9) stay where they were: moving the strongest
  early weapon into levels 1-5 would upset the first ten levels. Levels 1-5 get the dagger (lvl2) and the sling (lvl3)
  instead of club copies. Then the throwing stick lvl7, javelin 12 and 19, khopesh 14 and 16, epsilon axe 17, 21 and
  28, mace 20, 24 and 29, duckbill axe 22, 26 and 30, composite bow 23, 25 and 27 (each in place of a spear, bow,
  sword or club copy).
* **Bonus and mimic weapons.** A chest's bonus weapon and a mimic's weapon are now a weapon the player already holds,
  so a deep weapon never turns up early (13 weapons made the old "any weaker weapon" too wide).
* **Inventory.** The weapons tab scrolls by rows (wheel, arrow keys, a thin bar), as planned in
  [inventory-overhaul.md](solved/inventory-overhaul.md). The number row now picks the open tab's slots (the first 12):
  with 21 items one key per item no longer fits. The rest of [inventory-keys.draft.md](inventory-keys.draft.md) stays
  open.
* **Reach.** The dagger's 0.1 tiles reaches a monster at bite distance (`melee_weapons.txt`).
* Damage growth per level: the dagger 30%, the others 20% (club 40%, short sword 10% as before).

Tests: `ranged_weapons.txt`, `melee_weapons.txt`, `weapons_held.txt` (every weapon in the fist), unit tests for the
file ids, the old save and the loot.

Still open: the balance pass with [monster-balance.draft.md](monster-balance.draft.md) (the new weapons only have the
starting numbers above), and the stamina cost per attack.
