# Monster attack damage types

Status: draft 2026-10-06, proposal for review 2026-10-06. Needed by the typed damage amulets
([amulets.draft.md](amulets.draft.md)).

## Problem

Since [solved/damage-types-and-resistances.md](solved/damage-types-and-resistances.md) monsters resist or are weak to
blunt / slash / pierce, but their own hits on the player are untyped: `Player::TakeHit(int dmg, ...)`, armour takes a
flat amount (`PlayerStats::HitDamage`: `max(1, dmg - Armor)`). Nothing for a typed resistance to act on.

## Idea

* Each monster (and boss) gets a damage mix for its attack, like the weapons' mix in `ITEM_DEFS` (blunt, slash, pierce
  percent, sum 100). A new table `ATTACK_MIX_DEFS` next to `RESISTANCE_DEFS` (`src/state/assets.cpp`); a type not
  listed hits all blunt (or: a mix is required for every type, checked at load).
* `TakeHit` takes the damage with its mix; the player's resistances (from the amulet) apply per type, then the armour.
* The journal shows what a creature deals, written down when it first hits the player.

## Proposed mix

Read from what each model attacks with. `damage` is today's `MONSTER_DEFS` value.

| Monster | Damage | Attack | Blunt | Slash | Pierce |
|---|---|---|---|---|---|
| Worm | 9 | grinding maw | 60 | 40 | 0 |
| Scarab | 2 | mandibles | 0 | 70 | 30 |
| Giant scarab | 12 | jumps and rams, mandibles | 50 | 50 | 0 |
| Boss scarab | 40 | rams, mandibles | 60 | 40 | 0 |
| Man-eater plant | 5 | bite, thorny vines | 0 | 40 | 60 |
| Rat | 2 | teeth | 0 | 30 | 70 |
| Giant rat | 8 | teeth | 0 | 30 | 70 |
| Bat | 3 | fangs, claws | 0 | 20 | 80 |
| Giant bat | 10 | fangs, claws | 0 | 20 | 80 |
| Vampire bat | 80 | fangs | 0 | 0 | 100 |
| Mummy | 20 | fists | 100 | 0 | 0 |
| Anubis | 30 | was-sceptre | 100 | 0 | 0 |
| Anubis boss | 110 | was-sceptre, its forked foot | 60 | 0 | 40 |
| Crocodile | 26 | crushing jaws | 50 | 0 | 50 |
| Scorpion | 3 | claws, sting (poison) | 0 | 30 | 70 |
| Mimic | 10 | lid slam, teeth | 40 | 0 | 60 |

What it means for the amulets: pierce is the most common (rats, bats, scorpions, plant, crocodile), blunt the big
hitters (mummy, Anubis, boss scarab), slash rarely more than a share. A pierce amulet is the generalist, a blunt one
the boss pick.

## Rules (proposed)

* **Order:** type resistance first, then armour. The resistance scales the raw hit per part of the mix
  (`dmg * sum(mix_i * resist_i) / 10000`, rounded), armour then takes its flat amount, at least 1 as today. Armour
  after keeps a resistance worth the same share on weak and strong hits; armour first would make it worth almost
  nothing against small biters.
* **Player resistances:** `Resistances` like the monsters' (`src/world/damage.h`), all `NORMAL` without an amulet. The
  amulet values (a share off one type) belong to [amulets.draft.md](amulets.draft.md).
* **Poison** stays apart: the scorpion's sting typed as above, its poison untouched by type and armour, as today.
* **Traps** stay untyped (`TakeHit` without a mix: no type resistance, armour as today; the crushing rock still ignores
  armour). A trap type of its own for the trap amulet is for [amulets.draft.md](amulets.draft.md).
* **Without an amulet nothing changes:** all `NORMAL`, every hit deals what it does today. No balance pass needed.
* **Journal:** like the resistances it learns per weapon tried (`src/ui/journal_view.cpp`), on a creature's first
  hit its page gets a line like "Hits: pierce, some slash" (the major part, then shares of 30% or more as "some").
* **levelcheck:** nothing to check.

## Open

* The mix per monster: the table above is a proposal.
* A missing mix: default all blunt, or a load error?
* The journal wording, and whether it shows exact percentages.
