# Skill upgrades: a second heka on a learnt skill

Status: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-16, size M, needs nothing first.

## Idea

The user: once a skill is learnt, some skills can be upgraded by spending one more heka. The second rank adds
75%..125% of the first rank's strength (per skill, the implementer's balance call within that band).

Why it fits: the campaign gives 10 heka for 18 skills (`SKILL_COUNT`), so a run already picks breadth. Rank II adds a
depth choice: double down on a build (a mace stunner, an archer) instead of one more side skill. At 100% a rank II is
worth a fresh skill, so the choice is about synergy, not value.

## Plan

* Rank: a learnt skill is rank I; one more heka makes it rank II. Max rank II. No respec. Only skills with a row in
  the table below can be upgraded; the others never show an upgrade.
* `Heka`: a second bitmask `upgraded` (bit per `SkillId`), `Rank(id)` 0/1/2, `CanUpgrade(id)` (learnt, upgradable,
  not upgraded, a heka free), `Upgrade(id)`. `Spent()` counts both masks. `Waiting()` is also true when only an
  upgrade is left to buy. Saved on the `HEKA` line as one more field; a save without it loads as no upgrades.
* Every place that reads `Has(SkillId::X)` for an upgradable skill reads the rank instead (a helper per value, e.g.
  `dodgePercent(heka)`), so rank I values stay exactly as today.
* Chances near a cap: the rank II gain is a share of the failure left, not of the rank I gain, so nothing reaches a
  sure 100%.
* Heka screen (`ui/skills_*`, [ui.md](../ui.md)): a learnt upgradable card shows "I" and, with a heka free, the
  upgrade action (same key/click as learning) and the rank II effect line; an upgraded card shows "II". No new icons:
  the rank is a numeral on the card. Lore text per rank II in `content/texts/skills.txt`.
* Scenario `learn` command: `learn <skill> 2` (or `upgrade <skill>`) for tests and play scripts.
* Glossary: a row "skill rank" (rank I, rank II, the upgrade on the Heka screen). Not to be mixed with an amulet's
  tier or its shrine Upgrade: name the button "Deepen" or "Rank II" if "Upgrade" reads as the shrine's.

### Proposed rank II values

Starting points; tune in the play-test.

| Skill | Rank I (today) | Rank II proposal | Share |
|---|---|---|---|
| Ka of Osiris | max health +25% | +50% | 100% |
| Strength of Montu | Might +5 | +10 | 100% |
| Djed of Ptah | Armor +5 | +10 | 100% |
| Breath of Shu | max stamina +25% | +50% | 100% |
| Speed of Mafdet | walk, sprint +20% | +35% | 75% (movement and collision feel; the jump reach stays) |
| Dodge | 10% | 20% | 100% |
| Sharp eye | ranged reach 130% | 160%, no minimum range ([skill-eye-of-horus](skill-eye-of-horus.feature.md)) | 100% |
| Stun | 25% of the blunt share | 50% | 100% (boss still half, not again within 5 s) |
| Bleed | 20% of the cut share | 40% | 100% (tier-up halves follow it) |
| Flaming arrows | +40% fire, 10% stamina | +80% fire, same cost | 100% |
| Power hit | +92 x ln(1 + p/20) % | +161 x ln(...) % | 75% |
| Quiver of Neith | +40% shots, 25% faster | +70% shots, 44% faster | 75% |
| Serket's favour | 10% resist, half as long | 20% resist, a third as long | 100% resist |
| Blessing of Isis | 10% resist, half as long | 20% resist, a third as long | 100% resist |

No rank II (the user, 2026-11-16): Words of Thoth, Wisdom of Seshat, Favour of Hapi, Disarm trap.

### Open points (implementer's choice)

* The exact share per skill within 75%..125%; a weak, niche skill may get 125%.
* Whether a rank II may be bought on the same level as rank I (default: yes, no gate).

## Implementation notes

* Values: the table above, in `level/skills.h` (`*_2` constants). Chances (Dodge, Stun, Bleed, Serket's and Isis's
  resist): `rankTwoChance`, rank I plus rank I's share of the failure left: Dodge 10% -> 19%, a mace's stun 25% ->
  43%, the resists 10% -> 19%. Serket's favour and the Blessing of Isis: a third as long (`GOD_SKILL_DIVISOR_2`);
  their shrines stay twice as long at both ranks. Power hit: the curve's scale x1.75. Eye of Horus: 160%, no minimum
  range ([skill-eye-of-horus](skill-eye-of-horus.feature.md)).
* No gate: rank II can be bought right after rank I.
* Heka screen: the button reads "Rank II" (not "Upgrade", the shrine's word), confirmed by a second press like Learn.
  An upgradable learnt card shows "I" or "II" left of its seal (bright while a heka can deepen it); an upgraded card
  shows rank II's effect line; the papyrus shows rank II's effect and lore (`rank2` in `content/texts/skills.txt`).
* `learn <skill> 2`: learns it (if not yet) and its rank II. `expect skills` counts skills learnt, not heka spent.
* Leaderboard: a rank II skill is listed with " II".

## Play-test

* `bin/game --play tests/play/skill-upgrades.txt`: Dodge and Stun learnt, a mace, four heka free.
* Learn Dodge, then upgrade it on the Heka screen: the card shows II, two heka spent, the HUD tile stops pulsing
  when nothing is left to buy.
* With Stun II and a mace, monsters are stunned about twice as often as with Stun I.
* Save and load: the rank stays.
