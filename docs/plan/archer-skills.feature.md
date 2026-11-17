# Archer skills: sharp arrows, poison arrows, poison blade

Status: implemented 2026-11-17, waiting for the user's play-test. Was: draft 2026-11-16, size M, needs nothing first.

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
* Stacking with the venom amulet (which poisons melee and missile hits), from the user: the chances add, one roll
  (skill 20% + amulet 20% = 40%). A hit that poisons takes the stronger tier of the two. Sharp arrows' bleed and the Bleed skill: one roll, the higher chance.

### Open points (implementer's choice; warn the user)

* Flat cut is large next to the Nubian bow (7 base; Chariot bow 22): rank I's +5 is +70% early. Tune on a play-test.
* The poison chance: not given. Default 20%.
* Many late monsters are poison immune (mummy, Anubis guard, Anubis boss, plant, mimic): the poison skills fade in the
  necropolis. Accepted, it is a choice like the others.

## Implementation notes

* Names as proposed (Arrows of Satet, Sting of Serket, Fang of Wadjet); `SkillId::SharpArrows`, `PoisonArrows`,
  `PoisonBlade` appended, `SKILL_COUNT` 21. Rules in `src/level/archer_skills.h`, numbers in `gameplay_skills.h`.
* Chosen: poison arrows are **bows only** (`MissileKind::Arrow`), like sharp arrows and the "arrows" in the name; the
  sling, stick and javelin only get the venom amulet. Poison chance 20% (both ranks), cut +5 / +10 as given.
* `Dungeon::playerHit` got `HitBy` (Melee, Bow, Other) to tell the sources apart.
* A boss bleeds from sharp arrows at half the chance, as with the Bleed skill (the draft did not say).
* Sharp arrows' bleed: one pass with the Bleed skill, per tier the higher chance; rank II starts at medium
  regardless of an earlier weak bleed, the strong tier follows on a medium bleeder at half.
* The cut is added when the shot is made (before fire arrows and flaming arrows), so the shot's mix has a cut share:
  the Bleed skill also rolls on it (its own chance, the higher counts).
* Not done: nothing left out. Boss rule (AGENTS.md) does not apply: player skills.

## Play-test

* `bin/game --play tests/play/archer-skills.txt`: all three skills at rank II, a bow, a sling and a sword in the bag.
* Shoot the giant bat with the bow: it bleeds now and then (medium), poisons (strong); with the sword: poisons strong.
* The sling poisons and bleeds nothing. The mummy (far right) never bleeds and is never poisoned.
* Compare an arrow's damage with and without Arrows of Satet (+10 cut): clearly more on the bat.
