# Skill: blessing of Isis

Status: draft 2026-10-10. Needs [skills](skills.draft.md) and [curses](curses.draft.md) first. From the user.

## Idea

Isis, the great magician, shields the player from curses: **curses last half as long** (2 minutes → 1 minute). The
twin of [Serket's favour](skill-serkets-favour.draft.md) for poison.

* Applies when a curse starts (`Curse(kind, grade, ms)` sets half the time); a repeat curse restarts at half too.
* The HUD curse icon shows the halved seconds.
* Card text: "Blessing of Isis: curses on you last half as long." Journal note, humour welcome: "Isis has a knot for
  every curse. She ties fast."

## Tests

* Unit: a curse's time is halved with the skill.
