# Heka screen: long skill effects wrap to 2 lines on the card

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-15, size S, needs nothing first.

## Repro

1. Open the Heka screen (K).
2. Look at the skill cards in the list on the left, e.g. Quiver of Neith ("Ranged weapons hold 40% more shots, back
   25% faster"), Serket's favour, Blessing of Isis.

## Expected

A skill's effect line (`SkillDef::effect`, `src/level/skills.cpp`) under its name fits the card: when it is too wide
for one line it wraps to 2 lines, inside the card, clear of the icon well and the learnt / sealed mark (top right).
Short effects stay one line, as now.

## Actual

`SkillsScreen::DrawCard` (`src/ui/skills_view.cpp`) draws the effect as one `small` line at `CARD_EFFECT_Y`, whatever
its width: a long one runs past the card's right edge (or under the next card).

## Fix notes

* Wrap with the existing `wrap(font, text, width)` (as `DrawDetails` does), width from `textX` to the card's right
  edge minus a pad. Never more than 2 lines: if a text still needs 3, shorten that effect text instead.
* Two lines need room: move the name up and/or tighten the line gap for 2-line cards (`CARD_NAME_Y`,
  `CARD_EFFECT_Y`), or lay out every card for 2 lines so the names line up. Implementer's choice; keep the look of
  [docs/ui.md](../ui.md).
* A unit test over all `SKILLS`: every effect wraps to at most 2 lines at the card's text width (needs the font
  metrics; if that is not unit-testable, a scenario screenshot of the Heka screen instead).

## Fixed when

* A test that names the plan fails before the fix and passes after it.
* No effect text leaves its card; long ones show on 2 lines.

## Play-test

* `bin/game --play tests/play/heka-card-effect-two-lines.txt`, press K: Quiver of Neith, Serket's favour and Blessing of Isis show their effect on 2 lines inside the
  card; the short ones (Might +5, Armor +5) on one, names lined up.
