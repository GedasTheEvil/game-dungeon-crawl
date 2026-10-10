# Skills (Heka)

Status: draft 2026-10-10, ready (needs nothing first). From the user. The framework only; each skill has its own
sub-draft that needs this one first:
[skill-stun](skill-stun.draft.md), [skill-hieroglyphs](skill-hieroglyphs.draft.md),
[skill-dodge](skill-dodge.draft.md), [skill-power-hit](skill-power-hit.draft.md).

## Idea

A new screen beside the inventory, journal and map: **Skills**. Killing a boss gives one skill point; the player
spends it on a skill. Skills are permanent.

* Lore name for the point: **heka** (the Egyptian word for magic, the power the gods hold): "You take the heka of
  Sobek. 1 heka to spend." The screen title: "Heka" or "Skills" (implementer's choice; the glossary gets the term).
* Points: 1 per boss kill (lvl5, 10, 15, 20, 25; lvl30's boss ends the game, so 5 usable). Saved with the game.
* Each skill costs 1 point. No ranks for now (a later draft may add ranks or a tree).
* Scarce by design (user, 2026-10-10): there are more skills than points in a play-through, so each run picks a
  different few and plays a little differently. Aim for about twice as many skills as points over time; the four
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
