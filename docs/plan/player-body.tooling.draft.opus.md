# The player's position and motion in Player, not Dungeon

Status: draft 2026-11-16, size M, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 10): the player's position `mapX`/`mapY` lives in `Dungeon`
(`sim/dungeon.h`, used about 410 times in 28 sim files), with `sunkMs`, `walkDir`, `keysHeld` and about 30
`Dungeon::Player*()` queries (`PlayerWading`, `PlayerInSand`, `PlayerOnLadder`, `PlayerSink`, ...). `Player` itself
has about 20 public mutable fields (`jump`, `charge`, `crouchHeld`, `crouched`, `blindMs`, ...). Movement, crouch and
jump rules sit half in each, and nothing owns their invariants.

## Plan

* A `PlayerBody` (position, facing, walk direction, jump, crouch, sink) owned by `Player`.
* The `Dungeon::Player*()` queries become terrain queries that take a body (`TerrainsAt(body)`), usable for a
  monster as well.
* Mechanical first (move the fields, keep the behaviour; unit tests and the archived scenarios give the same results
  as before), then the queries.
* Do it before [dungeon-subsystems](dungeon-subsystems.tooling.draft.opus.md): it takes the player out of Dungeon's
  members.
