# Crocodile bite with a hold

Status: draft 2026-10-05, refined 2026-10-09 (decided, not implemented). Split off
[crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md); the crocodile ships with a plain bite first.

## Idea

The crocodile's bite grabs the player: a hold, a death roll.

## Decided (2026-10-09)

* **Where:** only while the player stands in water (a flooded cell). On land the crocodile bites as today.
* **When:** a bite turns into a hold at 30%, rolled on the gameplay stream.
* **Telegraphed:** before a grab the crocodile opens its jaws wide and waits 0.5 s, then lunges. The player can back
  off or jump away; a lunge that misses does nothing. A plain bite stays as it is.
* **The hold:** the grab does the bite's damage, then the death roll: a tick every 0.8 s of 40% of the bite's damage
  (about 10), 3 ticks, 2.4 s in all. Armour counts on each tick. Then the crocodile lets go.
* **While held:** the player can do nothing: no walking, jumping, sprinting, attacking or drinking (the quick-drink
  keys and the inventory too). No way to break free: the hold is short instead.
* Killing the crocodile while held (another source: a venom tick, a second crocodile does not) ends it.
* No amulet against it.

## Open (for the implementer)

* Look: the crocodile's open-jaws tell, the grab and the roll (Blender, [../remodeling.md](../remodeling.md)); the
  player's held view (camera shake or roll).
* The checker and the balance: the hold is short and only in water, so likely nothing. Note it with the crocodile's
  half-water weight in [monster-balance](monster-balance.draft.md).
