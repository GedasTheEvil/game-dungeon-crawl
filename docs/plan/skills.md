# Skills (Heka)

Status: draft 2026-10-10, implemented 2026-10-12 (see [Implemented](#implemented-2026-10-12)), waiting for
play-test. From the user. The framework only; each skill has its own sub-draft, built on this one:
[skill-stun](skill-stun.draft.opus.md), [skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md),
[skill-dodge](skill-dodge.draft.sonet.md), [skill-power-hit](skill-power-hit.draft.sonet.md),
[skill-sharp-eye](skill-sharp-eye.draft.sonet.md), [skill-blessing-of-hapi](skill-blessing-of-hapi.draft.sonet.md),
[skill-serkets-favour](skill-serkets-favour.draft.sonet.md), [skill-blessing-of-isis](skill-blessing-of-isis.draft.sonet.md).

## Idea

A new screen beside the inventory, journal and map: **Skills**. Killing a boss gives one skill point; the player
spends it on a skill. Skills are permanent.

* Lore name for the point: **heka** (the Egyptian word for magic, the power the gods hold): "You take the heka of
  Sobek. 1 heka to spend." The screen title: "Heka" or "Skills" (implementer's choice; the glossary gets the term).
* Points: 1 per boss kill (lvl5, 10, 15, 20, 25; lvl30's boss ends the game, so 5 usable). Saved with the game.
* Each skill costs 1 point. No ranks for now (a later draft may add ranks or a tree).
* Scarce by design (user, 2026-10-10): there are more skills than points in a play-through, so each run picks a
  different few and plays a little differently. Aim for about twice as many skills as points over time; the eight
  sub-drafts are the start. No respec, so a choice stays a choice.
* A boss killed again after a load does not give a second point (`slainMonster`).

## Screen

* A tab in the shared UI ([docs/ui.md](../ui.md)): opened with a key (default `K`) and from the screen tabs, like the
  map and the journal.
* A grid or list of skill cards: icon, name, one-line effect, a "learned" mark. Points left at the top. Click (or
  Enter) on a card learns it, with a confirm (points are scarce).
* An unlearned skill without points shows greyed, still readable.
* HUD: a small mark when a point is waiting to be spent.

## Code

* `PlayerStats` (or a `Skills` set beside it) keeps the learned skills and the free points; save format bump.
* Skills are a data table (id, name, effect text, icon), like `ITEMS` and `AMULETS`; each skill's rules live in its
  sub-draft.
* Field note on the first point, humour welcome: "Sobek's heka. It tastes of river mud."
* Glossary rows: heka / skill point, skill.

## Decided

* Points come from bosses only (decided, user 2026-10-10); none from player levels.

## Tests

* Unit: a boss kill gives one point, once; learning spends it; save and load keep both.
* Scenario with a screenshot of the screen: points free, one skill learned.

## Implemented (2026-10-12)

* Model: `Heka` (`src/world/skills.h`): the boss levels that gave a heka (so a boss killed again after a load gives
  none) and the skills learnt, saved after `PlayerStats`' RESIST line with a `HEKA` tag (old saves: none). The table
  `SKILLS` (name, card line, lore, `sealed`), saved by `SkillId` place: append only.
* A boss's death (`Dungeon::updateBoss`) gives the heka, a third status line "You take its heka" and the field note
  "Heka" (what heka is, the K page, choose with care: more skills than guardians).
* Screen: "Heka", the fourth screen tab (K, a flat ka glyph; tab label "Heka (skills)"; the four tabs shortened every
  tabbed title rule to 33). Cards 2 x 4 on a panel (icon well, name, one line), the heka left over them, details on
  papyrus with the Learn button: the first press asks ("Sure? Learn", "For good: no unlearning"), the second learns.
  Learnt: lapis card with a gold diamond; sealed: dark card with a red wax dot. Layout: `src/ui/skills_layout.h`.
* Icons baked: `textures/ui/skill_icons.png` from `tools/textures/skill_icons.py` (cell 0 the heka mark, then the
  skills). HUD: a pulsing lapis tile with the heka mark and the key above the panel's right end while a heka waits.
* Names picked: "Words of Thoth" (hieroglyphs), "Favour of Hapi" (the potion keeps "Blessing of Hapi").
* Every skill is **sealed** for now (shown, not learnable): their rules are the sub-drafts'. Each sub-draft clears its
  `sealed` flag. Scenario `learn` learns a sealed one for tests.
* Tests: `tests/unit/skills_test.cpp`, scenario `tests/scenarios/skills.txt` (HUD tile, screen, refusal, learnt).
