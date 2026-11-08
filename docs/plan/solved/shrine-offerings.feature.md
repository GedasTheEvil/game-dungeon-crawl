# Shrine offerings: merge rings and amulets, offer spares for XP

Status: solved 2026-11-08 (play-tested by the user). Was: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-11-08, size M, needs nothing first.

## Idea

The user: rings can be upgraded only at a god's shrine. Two rings of the same metal and gem are offered for one ring
of the next metal (copper → silver → gold); gold is the top and does not merge. Spare items can also be offered to
the god for XP: rings, amulets, and weapons, above all the copies of a weapon already at level 5 (`MAX_WEAPON_LEVEL`),
which have no use today. A confirmation asks before anything is given away.

Today weapons and amulets upgrade in the inventory, anywhere (`ItemBag::CanUpgrade`, `Upgrade`); rings never
upgrade. The user's rule: weapons are plain craft, the player joins the copies by hand, anywhere (unchanged). Amulets
and rings are magic, the player has none: they upgrade only at a shrine, with a god's help (Isis and the four gods,
[shrines-of-the-gods](shrines-of-the-gods.feature.md)). This gives the shrines a second use.

## Plan

* Offer happens from the inventory, not a new shrine screen: while the player stands at a shrine (the same reach as
  Pray), the item card gets an "Offer" button, "Merge" for a ring with a twin, and the amulet's "Upgrade". Away from
  a shrine Offer and Merge are hidden; the amulet's Upgrade shows disabled with a hint "Only at a shrine" (so the
  player learns where to go). Pray (E) on the shrine stays as is. Weapons upgrade anywhere, as today.
* Amulet upgrade: the same rules as today (`UpgradePoints`, `amuletWorth`, spares largest first), only the place
  changes: `CanUpgrade` for an amulet also needs the player at a shrine. The game's hints and the How to Play page
  that mention the amulet upgrade say so.
* Merge (rings): two rings of the same gem and metal, metal below gold → one ring of the next metal, same gem. Worn
  rings count only if a spare is left (like amulet spares: the worn one is not taken unless the player chooses it);
  a worn ring that merges stays worn on its hand as the new metal. Gold: no Merge button.
* Offer (any ring, amulet or weapon copy): the item leaves the bag, the player gains XP by its worth. What can be
  offered: a weapon only its copies past the 11 its level 5 took (decided in implementing: below level 5 a copy still
  counts towards the next level, and weapon copies are cumulative, so offering one would undo progress); an amulet a
  spare (not the worn one); a ring a spare (not a worn one).
* XP by worth (balance, the implementer's choice, tune by play): about a fraction of a normal kill at that depth,
  scaled by the item's rank: ring copper < silver < gold; amulet by `amuletWorth(tier)`; weapon by its base damage.
  It must not beat fighting: offering every spare of a level should give well under one player level.
* Confirmation: a yes / no box in the shared UI look ([ui.md](../../ui.md)): "Offer the Silver Lapis Ring to Ptah? It is
  gone for good." / "Merge two Copper Garnet Rings into a Silver Garnet Ring?". Enter / Esc.
* Feedback: the god's beam (`SunBeam::PRAYER_LIGHT` tint) and a status line "Ptah accepts the offering. +40 XP" /
  "Ptah joins the rings: Silver Garnet Ring".
* Optional, the implementer's choice: the god flavours the reward (Montu favours weapons, Ptah amulets, ...: +50% XP
  for those). Leave out if it bloats the plan.
* Glossary rows: offering, merge (ring); the amulet tier row: upgraded at a shrine. Unit tests for merge rules (gold
  cannot, worn ring stays worn, needs two), amulet upgrade (not away from a shrine, yes at one; a weapon anywhere)
  and offer rules (last copy cannot, worn cannot).

## Play-test

* `bin/game --play tests/play/shrine-offerings.txt`: lvl18, the Shrine of Isis 3 steps right; two lesser health
  amulets, two copper and two gold garnet rings, 13 short swords (U four times: level 5, two spares).
* Here, open the inventory (I): the amulet's Upgrade button says "At a shrine", greyed; the sword upgrades.
* Walk onto the shrine, I: under the item an Offer row (and Merge for a ring), the lore gone. Upgrade the amulet.
* Merge the copper rings: a yes / no box, then a Silver Garnet Ring. Gold: Merge greyed.
* Offer a gold ring and a spare sword: the box, the XP in the bar and the toast; close: the beam on the shrine.
