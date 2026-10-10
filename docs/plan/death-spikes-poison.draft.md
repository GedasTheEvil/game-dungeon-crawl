# Death spikes: strong poison

Status: draft 2026-10-10. Needs nothing first. From the user.

## Idea

The death trap (big spikes, `deathTraps` in the level checker, `SPIKE_ATTACK_MIX`) also applies strong poison
([stronger-poisons](stronger-poisons.md) tier `Strong`) to whoever it hurts. Plain spikes stay as they are.

* Applied by `Dungeon::updateTraps` on the damage tick, once per entry (not stacking per tick: re-applying
  refreshes at most, as `Player::Poison` does).
* Resistance (potion, amulet) rolls as for any poisoning. Trap ward (`trapCutPercent`) cuts the damage as before;
  decide whether 100% (immune) also blocks the poison (suggest yes).
* Monsters: reckless ones walking through get poisoned too (`Monster::Poison`, byPlayer false: no XP for the
  player's kill, as a trap's kill).
* Status line, e.g. "The spike tips are wet with venom." Maybe a green tint on the death trap model.
* Rhythm traps ([rhythm-traps](rhythm-traps.draft.md)) stay weak by default; a strong variant is allowed per
  trap. Consistent with this draft.

## Open points

* Balance: strong poison scales with max HP, so check the early death traps (levels with few HP) stay survivable;
  the level checker may need to count death-trap paths as dangerous.
* Antidote hints near death traps (a potion placed before them) is optional.

## Tests

* Unit: entering a death trap applies strong poison once; plain spikes do not; ward 100% blocks it.
* Scenario: walk through a death trap, screenshot the poisoned status.
