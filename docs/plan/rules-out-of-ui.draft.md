# Stage 2: item ids and game rules out of the UI

Status: planned 2026-09-30, not started. Stage 2 of the [code structure review](code-structure-review.draft.md).
Evidence: [the audit](code-structure-review-audit.draft.md) (Game logic in UI, Layering, Duplication).

## Why

* The item ids live in `ui/inventory.h`, which pulls in GL, fonts, models and SDL. So `world/loot.cpp`,
  `world/dungeon_monsters.cpp` and `ui/quick_potion.cpp` include a UI screen for three int constants, and
  `level_gen.cpp` and the editor copy the ids by hand (the editor's potion texts are already wrong: "+25 HP" where the
  potions heal a percentage).
* The rules (weapon level math, what can be used, what a potion does, the riddle's XP) sit in screens, so they can only
  be tested through a GL scenario, and the HUD, the inventory and the riddle each compute the XP progress themselves.
* The HUD's view model is built in `graphics/draw.cpp`; the quick drink and equip keys bypass `GameplayAction`.

## Steps

Each step keeps `make`, `make tidy` (with `make layers`), `make test` and `./levelcheck levels/lvl*` green.

1. **Unit tests.** Vendor doctest (`external/doctest/doctest.h`, one header). `tests/unit/*.cpp` build into
   `build/unit` against the libraries (no GL context, no window); `make unit` runs it, `make test` runs it first.
   First tests: `quickPotion`, and the level format round trip (`loadLevelFile` / `saveLevelFile`).
2. **Item ids, GL-free.** `src/world/items.h` with `ItemType`, `WeaponId`, `PotionId`, `InvSlot` (still int
   constants: they are saved in levels and save games; `enum class` is a separate decision, see Open questions),
   plus `PotionEffect` and the quick-drink constants from `quick_potion.h`. `inventory.h` includes it. `liblevel`
   gets it (the layer check needs a list of header-only files per library).
3. **Pure modules into `liblevel`.** `quick_potion.cpp` and `loot.cpp` (uses `rand()`, no GL) move into the library;
   `level_gen.cpp` and `tools/editor/tile_info.cpp` include `items.h` instead of their copies. The editor's potion
   texts come from one potion table (name, effect text) in `items.h`.
4. **Item rules.** `src/world/item_rules.{h,cpp}` (in `liblevel`): `weaponDamage(base, level)`, `upgradeCost`,
   `MAX_LEVEL`, `DAMAGE_PER_LEVEL`; `potionEffect(id, stats)` returning what to add (health, stamina, might, armour,
   max health) instead of `DrinkPotion`'s switch with magic 2 / 2 / 5; `canUse(slot, counts, equipped, hp, max hp,
   stamina, max stamina)` returning a reason enum. `Inventory` calls them and keeps the screen, the toast and the
   save I/O. Unit tests for each.
5. **Stats without the inventory.** `PlayerStats::Damage(weaponDamage)` takes the weapon damage from the caller
   (`updateAttack`) instead of reading `Game().ui.inventory`. `PlayerStats::LevelProgress()` replaces the three XP
   ratio copies (`draw.cpp`, `inventory.cpp`, `riddle.cpp`); the riddle's XP reward becomes
   `riddleReward(level)` next to it. Unit tests.
6. **Save slots out of `MainMenu`.** `src/state/save_slots.{h,cpp}`: slot file names, labels, the name list
   (`saves/gamelist.dat`), `slotInfo`. `MainMenu` only draws and calls it. `exit(666)` becomes a normal quit.
7. **HUD view model into `ui/`.** `quickSlot`, `weaponIcon`, `playerHudView` move from `graphics/draw.cpp` to
   `ui/player_hud_view.cpp`; `draw.cpp` calls `PlayerHud::draw(playerHudView(), ...)`.
8. **Keys through `GameplayAction`.** `QuickHeal`, `QuickStamina`, `Equip1..4` actions; `input.cpp` maps the keys
   (the hotkey table moves from `Inventory::IsQuick*Key` to `input_actions.h`); the HUD key caps read the same table;
   scenarios get `drink health|stamina` and `equip N` instead of raw `key`.

## Tests

* Unit: quick potion, level round trip, weapon damage / upgrade cost per level, every potion effect, `canUse` for each
  reason, `LevelProgress`, `riddleReward`, save slot labels.
* Scenarios: the existing `inventory`, `quick_potions`, `player_hud*`, `menu`, `riddle*`, `loot` must pass unchanged
  (they cover the moved code). Screenshot diffs of `player_hud` before and after step 7.

## Open questions

* `enum class` for the ids: safer (no mixing a weapon id with a potion id), but every save / level read needs a
  checked conversion. Do it in this stage, or leave the int constants?
* Should `Inventory` split into an item model (counts, levels, equipped; in `liblevel`) and the screen? That is the
  bigger version of step 4; it also moves the save format out of the UI.
