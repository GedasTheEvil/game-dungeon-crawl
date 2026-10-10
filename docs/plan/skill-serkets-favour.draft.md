# Skill: Serket's favour

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first. From the user.

## Idea

Serket, the scorpion goddess, shields the player from poison: **poisons last half as long** on the player (the same
damage per second, half the time). A twin of [Blessing of Isis](skill-blessing-of-isis.draft.md) for curses.

* Stacks with the resistance roll (amulet + potion, `PlayerStats::PoisonResistPercent`): that roll decides whether a
  poison takes; the skill halves the ones that do.
* With [stronger-poisons](stronger-poisons.draft.md): the strong tier's 19 s become ~10 s, so a strong poison left
  alone no longer kills at high max HP. Intended: the skill is the third way past poison (cure, ward, endure).
* Name clash: the poison amulet's flavour is "Serket's scorpion" (and the egyptian-names draft names it so). Fine:
  the amulet wards, the skill shortens. Card text: "Serket's favour: poisons on you last half as long."
* Journal note, humour welcome: "Serket likes me. Her children still bite, but they do not mean it as much."

## Tests

* Unit: a poison's time is halved with the skill, damage per second unchanged; the resistance roll unchanged.
