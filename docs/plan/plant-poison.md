# Plant poison: venomous and poisonous

Status: implemented 2026-10-10, awaiting user verification. From the user: the man-eater plant (glyph `p`,
`MonsterPlant`) gets a chance to poison the player, both when it bites (venomous) and when the player hits it
(poisonous). Weak poison.

## Idea

* **Venomous:** a bite has a chance to poison the player, weak tier. Default 30%.
* **Poisonous:** a melee hit by the player on the plant has a chance to poison the player, weak tier. Default 20%.
  Thorns and sap: only melee weapons touch it. Arrows, the sling, the javelin and the throwing stick do not
  poison the player. A good reason to shoot it.
* The player's poison resistance (potion, amulet) rolls as for any poisoning (`Player::Poison`).
* A status line on the touch poisoning, e.g. "Its sap burns".

## Sketch

* `MonsterKind` gets a poison chance for bites (`poisonChancePercent`, default 100 so the scorpion and the cobras keep
  poisoning on every hit) and a touch poison (tier + chance, nullopt for every other kind).
* The plant: `.poison = PoisonTier::Weak`, chance 30; touch: weak, 20.
* Description: "Man-eater plant, its bite and its sap poison (weak)". Note keeps the humour, e.g. "Rooted to the
  spot. Never stand next to it. Never touch it, and hitting it counts."
* The journal records the touch poisoning like a bite (`Journal::TryPoison`).
* Checker: the plant becomes a poisoner (`isPoisoner`), so its levels want an antidote in reach. Fix every level the
  checker flags, levels 1-5 included (AGENTS.md).
* No boss has the plant as kin; no boss change.

## Related

* [stronger-poisons](stronger-poisons.md) leaves the weak tier unchanged; no conflict.
* [egyptian-names](egyptian-names.draft.md) renames the plant (Thorn acacia); whichever comes second uses the
  other's name.

## Tests

* Unit / sim: a bite poisons at about the chance (seeded rolls), a melee hit on the plant poisons the player at about
  the chance, a ranged hit never does, the resistance potion blocks it, other poisoners still poison every hit.
* `./levelcheck levels/lvl*`: no warnings.
