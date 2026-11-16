# Archer skills: sharp arrows, poison arrows, poison blade

Status: draft 2026-11-16, size M, needs nothing first.

## Idea

From the user: three Heka skills (names may change):

* **Sharp arrows:** arrows (bows only) do extra cutting damage and may make the foe bleed (unless it is immune).
  Rank I: +5 cut, a 20% chance of a bleed. Rank II: +10 cut in total, and the bleed starts at tier 2 (medium).
* **Poison arrows:** a chance to poison. Rank I: medium poison, rank II: strong poison.
* **Poison blade:** the same for melee weapons.

## Plan

* Three `SkillId`s appended (`src/level/skills.h`, saved by place: never reorder), `SKILL_COUNT` 18 -> 21, rows in
  `SKILLS` (`src/level/skills.cpp`), lore and `rank2` lore in `content/texts/skills.txt`, three icon cells in
  `tools/textures/skill_icons.py`, field notes (`skillNote`), the manual's skill list. Follow Bleed and Stun
  ([skill-bleed](solved/skill-bleed.feature.md), [skill-stun](solved/skill-stun.md)).
* Names, Egyptian like the others (proposal): Sharp arrows = **Arrows of Satet** (Satet, "she who shoots"; Neith is
  taken by the Quiver). Poison arrows = **Sting of Serket**. Poison blade = **Fang of Wadjet**.
* **Sharp arrows:** bows only (Nubian bow, Chariot bow; not the sling, stick, javelin). Cut added to the shot's
  damage, through the monster's cut resistance: rank I +5, rank II +10 in total. Bleed: 20% per hit that does not
  kill (same chance at both ranks), then the monster's own cut resistance / bloodless rule (`bleedChancePercent`, as
  Bleed does). Tier: rank I `BleedTier::Weak`, rank II `BleedTier::Medium`.
* **Poison arrows / poison blade:** a hit that does not kill rolls, then the monster's `poisonResistPercent` (as the
  venom amulet, [venom-amulet](solved/venom-amulet.md)). Rank I medium, rank II strong (`skill-upgrades` rank II).
  Chance: 20% (open point). Poison arrows: every ranged weapon (bows, sling, stick, javelin) or bows only (open point);
  poison blade: melee hits only.
* Stacking with the venom amulet (which poisons melee and missile hits): roll once, the stronger tier wins (default),
  not two rolls. Sharp arrows' bleed and the Bleed skill: one roll, the higher chance.

### Open points (implementer's choice; warn the user)

* Flat cut is large next to the Nubian bow (7 base; Chariot bow 22): rank I's +5 is +70% early. Tune on a play-test.
* The poison chance: not given. Default 20%.
* Many late monsters are poison immune (mummy, Anubis guard, Anubis boss, plant, mimic): the poison skills fade in the
  necropolis. Accepted, it is a choice like the others.

## Play-test

* `tests/play/archer-skills.txt` with the three skills and a bow, a sword: shoot a giant bat, it bleeds now and then (weak at rank I,
  medium at rank II);
  poison arrows and poison blade poison it, medium at rank I, strong at rank II.
* Shoot a mummy: never bleeds, never poisoned.
