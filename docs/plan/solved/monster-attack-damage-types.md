# Monster attack damage types

Status: solved 2026-10-06 (play tested). Draft 2026-10-06, refined 2026-10-06. Needed by the typed
damage amulets ([amulets.draft.md](../amulets.draft.md)).

## Problem

Since [damage-types-and-resistances.md](damage-types-and-resistances.md) monsters resist or are weak to
blunt / slash / pierce, but their own hits on the player are untyped: `Player::TakeHit(int dmg, ...)`, armour takes a
flat amount (`PlayerStats::HitDamage`: `max(1, dmg - Armor)`). Nothing for a typed resistance to act on.

## Idea

* Each monster group (bosses included) and each trap gets a damage mix for its attack, like the weapons' mix in
  `ITEM_DEFS` (blunt, slash, pierce percent, sum 100). A new table `ATTACK_MIX_DEFS` next to `RESISTANCE_DEFS`
  (`src/state/assets.cpp`). Every group has one: a new group's mix is part of its design, written in its plan.
* `TakeHit` takes the damage with its mix; the player's resistances (from the amulet) apply per type, then the armour.
* The journal shows what a creature deals, written down when it first hits the player.

## Mix

One mix per monster group: the members of a group (the same model, bigger or darker) deal the same kinds of damage,
only a different amount. Read from what the model attacks with. `damage` is today's `MONSTER_DEFS` value.

| Group | Members (damage) | Attack | Blunt | Slash | Pierce |
|---|---|---|---|---|---|
| Worm | worm (9) | grinding maw | 60 | 40 | 0 |
| Scarab | scarab (2), giant scarab (12), boss scarab (40) | rams, mandibles | 40 | 60 | 0 |
| Plant | man-eater plant (5) | bite, thorny vines | 0 | 40 | 60 |
| Rat | rat (2), giant rat (8) | teeth | 0 | 30 | 70 |
| Bat | bat (3), giant bat (10), vampire bat (80) | fangs, claws | 0 | 20 | 80 |
| Mummy | mummy (20) | fists | 100 | 0 | 0 |
| Anubis | Anubis (30), Anubis boss (110) | was-sceptre, its forked foot | 80 | 0 | 20 |
| Crocodile | crocodile (26) | crushing jaws | 50 | 0 | 50 |
| Scorpion | scorpion (3) | claws, sting (poison) | 0 | 30 | 70 |
| Mimic | mimic (10) | lid slam, teeth | 40 | 0 | 60 |

The table keyed by group (the model), not by monster type, so a new member gets its group's mix. A new group brings
its own row, defined in its plan: the cobra and Apep bite (pierce), the scorpion queen maybe all three (claws, sting,
a crushing tail).

| Trap | Blunt | Slash | Pierce |
|---|---|---|---|
| Spikes, death trap | 0 | 20 | 80 |
| Crushing rock (graze and crush) | 100 | 0 | 0 |

What it means for the amulets: pierce is the most common (rats, bats, scorpions, plant, crocodile, spikes), blunt the
big hitters (mummy, Anubis, boss scarab, the rock). A pierce amulet is the generalist, a blunt one the boss pick.

## Rules

* **Order:** type resistance first, then armour. The resistance scales the raw hit per part of the mix
  (`dmg * sum(mix_i * resist_i) / 10000`, rounded), armour then takes its flat amount, at least 1 as today. Armour
  after keeps a resistance worth the same share on weak and strong hits; armour first would make it worth almost
  nothing against small biters.
* **Player resistances:** `Resistances` like the monsters' (`src/world/damage.h`), all `NORMAL` without an amulet. The
  amulet values (a share off one type) belong to [amulets.draft.md](../amulets.draft.md).
* **Poison** stays apart: the scorpion's sting typed as above, its poison untouched by type and armour, as today.
* **Traps** are typed too (table above): a type resistance applies to them. Armour as today: the spikes take it, the
  crushing rock still ignores it. The trap amulet ([amulets.draft.md](../amulets.draft.md)) stacks on top.
* **Without an amulet nothing changes:** all `NORMAL`, every hit deals what it does today. No balance pass needed.
* **Journal:** like the resistances it learns per weapon tried (`src/ui/journal_view.cpp`), on a creature's first
  hit its page gets one sentence in the explorer's voice. It names the most hurting type; the smaller shares, if any,
  only as "a hint of". No percentages. See [Journal wording](#journal-wording).
* **levelcheck:** nothing to check.

## Journal wording

One sentence, built from the mix: the main phrase for the largest share, then "with a hint of ..." for each smaller
share (joined with "and"). An even split names both as the main.

| Type | Main phrase | Hint |
|---|---|---|
| Blunt | hits me bluntly hard | crushing pain |
| Slash | cuts me deep | cutting pain |
| Pierce | pierces me to the bone | piercing pain |

Even split (blunt and pierce): "crushes and pierces me alike".

What the groups would read:

| Group | Sentence |
|---|---|
| Worm | It hits me bluntly hard, with a hint of cutting pain. |
| Scarab | It cuts me deep, with a hint of crushing pain. |
| Plant | It pierces me to the bone, with a hint of cutting pain. |
| Rat | It pierces me to the bone, with a hint of cutting pain. |
| Bat | It pierces me to the bone, with a hint of cutting pain. |
| Mummy | It hits me bluntly hard. |
| Anubis | It hits me bluntly hard, with a hint of piercing pain. |
| Crocodile | It crushes and pierces me alike. |
| Scorpion | It pierces me to the bone, with a hint of cutting pain. |
| Mimic | It pierces me to the bone, with a hint of crushing pain. |

Same mix, same sentence (rat, bat, scorpion, plant): accepted for now. Per-group lines (the bat: "Its fangs pierce me
to the bone, with a hint of cutting pain.") only if the general ones read badly in play.

## Open

Nothing; ready to implement. The mix values get revisited in play, together with
[monster-balance.draft.md](../monster-balance.draft.md).

## Done

* `ATTACK_MIX_DEFS` (`src/state/assets.cpp`), keyed by the model; a monster type without a row logs an error.
* `SPIKE_ATTACK_MIX`, `ROCK_ATTACK_MIX`, `playerHitDamage` and `attackSentence` in `src/world/damage.h`.
* `Player::TakeHit(dmg, mix, ...)`; `PlayerStats::resist` all `NORMAL`, not saved until the amulets set it.
* The journal's SAW entry starts with the sentence once the creature has hit the player.
* Unit tests: `tests/unit/damage_test.cpp`.
