# Bleeding: a cut condition in three tiers for the player and monsters

Status: draft 2026-11-15, size L, needs nothing first.

## Idea

Cut damage has no side effect of its own today: blunt has stun, fire has burning, and poison has its tiers. Bleeding
gives cut one. It is a neutral condition, the same for the player and the monsters (`Vitals`, plan shared-vitals),
in three tiers (weak, medium, strong) like poison (`POISON_TIERS`, `src/level/poison.h`).

## Plan

* **Condition**: a `Bleeding` part in `Vitals`, three tiers with their own timers and tick rates, built like `Poison`
  (medium and strong scale with the victim's max HP above a floor). The tier values are the implementer's balance
  call. Base them on poison's, a bit weaker, since healing stops bleeding.
* **Ticks**: a tick deals cut damage but goes through neither armour nor resistance. Once a body bleeds, resistance
  no longer matters: resistance only decides whether the bleed lands (see Chance). A bleed kill is credited like a
  poison kill (`HarmSource`; the player's doing gives XP, a trap's gives none).
* **Chance**: each source has a base chance and a tier. The victim's cut resistance scales the chance by its rate
  (`resist[Slash]`: resists 50% halves it, immune 0% stops it, vulnerable 200% doubles it, capped at 100%). The
  player's cut resistance includes the Cut Guard amulet (`AmuletType::SlashWard`).
* **Sources**:
  * Monsters whose attack has a cut share (`DamageMix`, slash > 0) get a chance per hit. The chance and tier come from
    the kind (`MonsterKind`), weighted by the cut share. A boss whose `kin` cuts gets a higher tier and/or chance
    (AGENTS.md, Boss abilities).
  * All spikes: floor spikes, the death trap, wall spikes, ceiling spikes, the ceiling death trap. Spikes deal stab
    damage, but the cut resistance still decides the chance (one bleed resistance only). The trap ward stops it like
    it stops the trap (`TrapWardPercent`), and the trap share (`trapPermille`) of 0 means no bleed.
  * Not on a dodge (`Player::Dodges`): no damage, no bleed.
* **Bloodless monsters**: a `MonsterKind::bloodless` flag, never bleed: mummies, ushabti, the mimic jar and the cursed
  mimic, the plant, stone or clay things, skeletons/spirits if any. The implementer walks the full kind list
  (`src/level/monster_kinds.cpp`) and lists the choice in the commit.
* **Stopping it**: any heal ends every running tier: a health potion (any size), Life, a level up, the Grace of Amun.
  Regeneration does not cure it: the regeneration amulet, the garnet rings and the player's own regen give no HP
  while bleeding (as with poison). Monsters only bleed out (no potions); a monster's own regen, if any, stops too.
* **Water**: in water a bleed's time runs 50% slower (it lasts 50% longer), half and deep water alike. The opposite
  of fire, which half water puts out. Water monsters bleed longer in their own water too; that is intended.
* **Shown**: a grey condition status card with drop pips by tier (`status_list.h`, `addConditions`), and the same
  icon over a monster's health bar (plan monster-status-icons). A few red drops fall from a bleeding body while it
  bleeds (a cheap particle, no new model). A message the first time ("You are bleeding!") and a field note.
* **Checker**: where something can make the player bleed, the checker wants a health potion to be reachable, like
  it wants an antidote for poison. Check that it adds no warnings to the campaign levels; fix the levels if it does.
* **Saved**: the running tiers, like poison.
* Glossary rows: bleeding, bleed tier, bloodless.

## Play-test

* `tests/play/bleeding.txt`: a level with a cutting monster, floor spikes and some water. Get hit until you bleed:
  the card shows up, HP drops, drops fall.
* Stand in water while bleeding: the card's seconds drop more slowly.
* Drink a health potion: the bleeding stops. While bleeding, the regeneration amulet gives no HP.
* Hit a mummy with a sword (with the skill-bleed draft, or a test hook): it never bleeds.
