# Weapon ranges and HP balance

Status: idea, not started. Follow-up of [monster hitboxes](solved/monster-hitboxes.md), whose bug fix is verified.

## What

Now that monsters and the player have real hitboxes, fights may be easier than before (big monsters are hit at
their near edge, not their centre). Review the reach of every weapon, then retune monster HP.

* Current reach: club 0.4, sword 0.5, spear 0.8 tiles, the bow's aim 3 tiles.
* Each weapon has a role: the club short and heavy, the sword quick with medium reach, the spear the longest melee
  reach (it can hit a monster before its bite lands), the bow at range.
* Check with the debug boxes (`hitboxes on`, F3) that each reach looks right against the model in the hand.
* Check in play and with scenarios: a small monster (scarab) and a big one (giant rat, boss scarab) against each
  weapon. The player should land hits at the distance where the monster bites; only the spear outreaches the bite.
* Then retune monster HP (`MONSTER_DEFS`, `BOSS_DEFS` in `src/state/assets.cpp`): replay lvl5 (boss scarab, 320 HP)
  and a giant rat level. Not before the reach review.

## Related

* Traps still check the player's point (`Trap::Hurt`), not the player box: left out on purpose in
  [trap-and-font-bugs.md](solved/trap-and-font-bugs.md). Revisit only if trap hits feel off.
