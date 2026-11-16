# Archer skills: sharp arrows, poison arrows, poison blade

Status: draft 2026-11-16, size M, needs nothing first.

## Idea

From the user: three Heka skills (names may change):

* **Sharp arrows:** arrows (bows only) do +10 cutting damage, and a hit has a 20% chance to make the foe bleed (unless
  it is immune).
* **Poison arrows:** a chance to poison. Rank I: medium poison, rank II: strong poison.
* **Poison blade:** the same for melee weapons.

## Plan

* Three `SkillId`s appended (`src/level/skills.h`, saved by place: never reorder), `SKILL_COUNT` 18 -> 21, rows in
  `SKILLS` (`src/level/skills.cpp`), lore and `rank2` lore in `content/texts/skills.txt`, three icon cells in
  `tools/textures/skill_icons.py`, field notes (`skillNote`), the manual's skill list. Follow Bleed and Stun
  ([skill-bleed](solved/skill-bleed.feature.md), [skill-stun](solved/skill-stun.md)).
* Names, Egyptian like the others (proposal): Sharp arrows = **Arrows of Satet** (Satet, "she who shoots"; Neith is
  taken by the Quiver). Poison arrows = **Sting of Serket**. Poison blade = **Fang of Wadjet**.
* **Sharp arrows:** bows only (Nubian bow, Chariot bow; not the sling, stick, javelin). +10 cut added to the shot's
  damage, through the monster's cut resistance. Bleed: 20% per hit that does not kill, weak tier (open point), then the
  monster's own cut resistance / bloodless rule (`bleedChancePercent`, as Bleed does). Rank II: optional (e.g. 35%
  bleed, `rankTwoChance`).
* **Poison arrows / poison blade:** a hit that does not kill rolls, then the monster's `poisonResistPercent` (as the
  venom amulet, [venom-amulet](solved/venom-amulet.md)). Rank I medium, rank II strong (`skill-upgrades` rank II).
  Chance: 20% (open point). Poison arrows: every ranged weapon (bows, sling, stick, javelin) or bows only (open point);
  poison blade: melee hits only.
* Stacking with the venom amulet (which poisons melee and missile hits): roll once, the stronger tier wins (default),
  not two rolls. Sharp arrows' bleed and the Bleed skill: one roll, the higher chance.

### Open points (implementer's choice; warn the user)

* +10 flat cut is large next to the bows (Nubian bow 7, Chariot bow 22 base): +140% early, +45% late. Alternatives: +5,
  or a percent of the shot. Default: as the user said, +10, tuned on a play-test.
* The poison chance: not given. Default 20%.
* Many late monsters are poison immune (mummy, Anubis guard, Anubis boss, plant, mimic): the poison skills fade in the
  necropolis. Accepted, it is a choice like the others.

## Play-test

* `tests/play/archer-skills.txt` with the three skills and a bow, a sword: shoot a giant bat, it bleeds now and then;
  poison arrows and poison blade poison it, medium at rank I, strong at rank II.
* Shoot a mummy: never bleeds, never poisoned.
