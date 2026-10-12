# Skill: Serket's favour

Status: draft 2026-10-10, implemented 2026-10-12 (see [Implemented](#implemented-2026-10-12)), waiting for play-test. From the user.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

Serket, the scorpion goddess, shields the player from poison: **poisons last half as long** on the player (the same
damage per second, half the time). A twin of [Blessing of Isis](skill-blessing-of-isis.draft.sonet.md) for curses.

* Stacks with the resistance roll (amulet + potion, `PlayerStats::PoisonResistPercent`): that roll decides whether a
  poison takes; the skill halves the ones that do.
* With [stronger-poisons](stronger-poisons.md): the strong tier's 19 s become ~10 s, so a strong poison left
  alone no longer kills at high max HP. Intended: the skill is the third way past poison (cure, ward, endure).
* Name clash: the poison amulet's flavour is "Serket's scorpion" (and the egyptian-names draft names it so). Fine:
  the amulet wards, the skill shortens. Card text: "Serket's favour: poisons on you last half as long."
* Journal note, humour welcome: "Serket likes me. Her children still bite, but they do not mean it as much."

## Tests

* Unit: a poison's time is halved with the skill, damage per second unchanged; the resistance roll unchanged.

## Implemented (2026-10-12)

* `Poison::Apply(tier, halved)` (`src/world/poison.cpp`): halved gives half the tier's `durationMs`, same damage a
  second. `Player::Poison` passes `stats.heka.Has(SkillId::SerketsFavour)` after the resistance roll (which is
  unchanged, no extra roll). A restart of a running tier also gets the half time. Shown seconds on the HUD follow.
* `sealed` cleared; card text unchanged ("Poisons last half as long").
* Journal note: new `FieldNote::SerketsFavour` (appended, `FIELD_NOTE_COUNT` 19), text in `journal_view.cpp`. Default
  picked: written when the skill is learnt on the Heka screen (`SkillsScreen::PressLearn`), not by the scenario
  `learn` command.
* Tests: `tests/unit/poison_test.cpp` (time halved, damage a second equal, totals about half),
  `tests/unit/skills_test.cpp` (the player's poison with the skill, no extra random roll).
