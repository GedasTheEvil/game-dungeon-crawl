# Head bump: a jump into a low ceiling hurts

Status: draft 2026-11-03. Needs first: [subcell-physics](subcell-physics.md). From the user.

Today a jump that meets a low ceiling (a solid sub-cell over the body, `Dungeon::ceilingAt`) stops the rise and plays
`WorldSound::HeadBump` (`src/world/dungeon_base.cpp`, the jump step). Now the bump also hurts and can daze.

## The bump

* 5 HP (from the user), straight off the HP: no armour, no resistances (the player's own head, no helmet slot).
  It can kill, like fall damage; the death line names the cause (pick the wording, e.g. "cracked their skull").
* 50% chance (from the user), rolled on the gameplay stream, of a daze for 2 to 3 s (from the user), uniform in that
  range. Reuse the screech's daze (`Player::Daze`, plan giant-bat-screech): half walk speed, no sprint, no jump,
  the view wobbles, the stars of Nut circle the head, the HUD daze tile. No new state.
* Only a real bump hurts: the rise must be cut short, the jump's upward velocity at the contact above a small
  threshold (pick it, e.g. a third of the take-off velocity). A jump whose top just grazes the ceiling only knocks
  (the sound, as today). This keeps hops under a ceiling only a little higher than the jump's top harmless.
* Once per jump: the rise ends at the bump, so it cannot repeat in the same jump. A dazed player cannot jump, so no
  chain of bumps while dazed.
* A pain grunt on a hurting bump, besides the knock (reuse the player's hurt sound).

## Tests

* Unit: a jump under a low sub-cell ceiling costs 5 HP; a graze below the threshold costs nothing; with a fixed seed
  a daze of 2..3 s follows about half the bumps (count over many rolls); dazed, no jump; a bump at 5 HP or less kills.
* Scenario on `tests/levels/subcells`: jump under the low ceiling, screenshot of the stars and the HUD tile.
* Glossary: "head bump".
