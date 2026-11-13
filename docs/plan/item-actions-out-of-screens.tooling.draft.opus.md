# Item, riddle and skill actions out of the screens into GL-free code

Status: draft 2026-11-13, size L, needs nothing first.

## Problem

Plan item-effects-out-of-ui (solved 2026-10-07) moved the potion's effect into `PlayerStats::Drink`. Since then the
amulets, rings, shrine offerings, ring merges, offering gifts, sky iron and the Hieroglyphs skill added their rules to
the screens again. What a player action does to the game (the bag, the stats, the journal, the run's counters, the
XP) now sits in three screen classes that use `Game()` and draw, so no unit test can reach it:

| Screen | The rules in it |
|---|---|
| `src/ui/inventory.cpp` | `Offer` (247: bag, `offerXP`, `Offered`, `AddXP`, `Gift`), `Gift` (264: `RollOfferingGift`, `run.offerings++`, `Grant` or `AddItem`), `Merge` (281: bag, the Sky iron note, `WearRings`, `Offered`), `WearOnHand` (300), `Upgrade` (313: `Wear` keeps the HP share), `Use` (337: `Wear`, `WearRings`), `DrinkPotion` (374: `run.Drink`), `QuickDrink` (390: the cooldown), `QuickAntidote` (411) |
| `src/ui/riddle.cpp` | `KeyboardF` (256-272): `journal.Solve`, `run.riddles++`, `AddXP`, the autosave |
| `src/ui/skills_view.cpp` | `PressLearn` (95-116): `Learn`, the skill's field note, Hieroglyphs: `TranslateInscriptions`, the XP per copy, the boons |

And `Inventory` owns the player's `ItemBag` (`Game().ui.inventory->Bag()`; the sim gets a pointer to it,
`SimLinks::items`, `src/sim/sim_links.h:20`).

What it costs today:

* **Copies that drifted.** The Hieroglyphs rule is written twice: `SkillsScreen::PressLearn` and the scenario command
  `learn` (`src/test/scenario.cpp:440-455`). The scenario's copy writes no field note for the skill
  (`skillNote`), the screen's does. The Sky iron note is written by `ItemBag::Find` (`src/level/item_bag.cpp:570`) and
  again by hand in `Inventory::Merge` (`inventory.cpp:290`), because `ItemBag::Merge` uses `Add`, not `Find`.
  `SimWorld::Wear` / `WearRing` (`tests/unit/sim_world.h:169-184`) copy `Inventory::Wear` / `WearRing`.
* **No tests.** The offering's XP plus gift plus counter, a merge's note and rings, a riddle's reward and counter, a
  skill's note and translation XP are tested only through their parts (`offering_gift_test.cpp`,
  `ring_accessories_test.cpp:417` tests `Find`, not the merge path).
* Every new item or skill rule lands in a 1343-line screen file (`inventory.cpp`, the largest in `src/ui/`).

## Plan

Incremental, one screen per commit, no gameplay change.

1. A GL-free place for the actions, in the sim library (it needs the `Dungeon` for the shrine and the level number):
   `src/sim/player_actions.{h,cpp}`, functions over a small struct of what they touch (`PlayerStats&`, `ItemBag&`,
   `Journal&`, `WorldEvents&`, the level number, the shrine's `God`). Each returns what the screen needs to show (the
   toast text, the item to select), and the sounds go out as `WorldEvents` as the sim's do.
2. Inventory: `Offer`, `Gift`, `Merge`, `Upgrade`, `Use`, `WearOnHand`, `DrinkPotion` move there; the screen keeps the
   selection, `KeepPlaces`, the confirm box, the toast. Fix the Sky iron copy by having `ItemBag::Merge` go through
   `Find`'s note (or one `noteNewItem` both call). `QuickDrink`'s cooldown may stay (it is HUD feedback too, as
   item-effects-out-of-ui decided).
3. Riddle: `SolveRiddle(...)` (journal, counter, XP); the screen keeps the screen switch, the toast and asks for the
   autosave.
4. Skills: `LearnSkill(id, ...)` returns the text parts; the screen and the scenario's `learn` both call it.
5. SimWorld's `Wear` / `WearRing` and the scenario's `Wear` / `Learn` call the same functions.
6. Unit tests through the new functions: an offering at a shrine adds `offerXP`, counts the offering and gives the
   seeded gift; a merge into sky iron writes the note once; a solved riddle adds its XP and `run.riddles`; learning
   Hieroglyphs with two read inscriptions adds twice `InscriptionXP` and the skill's note.

Optional, later: the `ItemBag` owned by `GameState` (beside the `Player`) instead of by the screen.

## Risk

Medium: many small moves in UI code with sounds and toasts. Keep each screen's visible behaviour; the scenarios for
the inventory, shrines, riddles and skills (`make test`) must pass unchanged.

## Verify

* `make check`, `make test`.
* `grep -n "Game()" src/ui/inventory.cpp | wc -l` drops (59 today); no `AddXP`, `Offered`, `run.` left in
  `inventory.cpp`, `riddle.cpp`, `skills_view.cpp`.

## Open questions

* sim library or level library: the level library is enough if the shrine's `God` and the level number come in as
  values. Implementer's choice.
* `Wear` / `WearRing` / `bagUser` overlap [player-control-in-sim](player-control-in-sim.tooling.draft.opus.md):
  whichever lands first takes them.
