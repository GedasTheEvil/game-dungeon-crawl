# Shrine offerings: merge rings and amulets, offer spares for XP

Status: draft 2026-11-08, size M, needs nothing first.

## Idea

The user: rings can be upgraded only at a god's shrine. Two rings of the same metal and gem are offered for one ring
of the next metal (copper → silver → gold); gold is the top and does not merge. Spare items can also be offered to
the god for XP: rings, amulets, and weapons, above all the copies of a weapon already at level 5 (`MAX_WEAPON_LEVEL`),
which have no use today. A confirmation asks before anything is given away.

Today weapons and amulets upgrade in the inventory, anywhere (`ItemBag::CanUpgrade`, `Upgrade`); rings never
upgrade. The user's rule: weapons are plain craft, the player joins the copies himself, anywhere (unchanged). Amulets
and rings are magic, the player has none: they upgrade only at a shrine, with a god's help (Isis and the four gods,
[shrines-of-the-gods](solved/shrines-of-the-gods.feature.md)). This gives the shrines a second use.

## Plan

* Offer happens from the inventory, not a new shrine screen: while the player stands at a shrine (the same reach as
  Pray), the item card gets an "Offer" button (and "Merge" for a ring with a twin). Away from a shrine the buttons
  are hidden or disabled with a hint "Only at a shrine". Pray (E) on the shrine stays as is.
* Merge (rings): two rings of the same gem and metal, metal below gold → one ring of the next metal, same gem. Worn
  rings count only if a spare is left (like amulet spares: the worn one is not taken unless the player chooses it);
  a worn ring that merges stays worn on its hand as the new metal. Gold: no Merge button.
* Offer (any ring, amulet or weapon copy): the item leaves the bag, the player gains XP by its worth. What can be
  offered: a weapon only its copies beyond the one held (never the last copy, never the equipped weapon's last); an
  amulet a spare (not the worn one); a ring a spare (not a worn one). Weapon copies below level 5 can be offered too,
  but the confirmation warns that they would raise the weapon's level.
* XP by worth (balance, the implementer's choice, tune by play): about a fraction of a normal kill at that depth,
  scaled by the item's rank: ring copper < silver < gold; amulet by `amuletWorth(tier)`; weapon by its base damage.
  It must not beat fighting: offering every spare of a level should give well under one player level.
* Confirmation: a yes / no box in the shared UI look ([ui.md](../ui.md)): "Offer the Silver Lapis Ring to Ptah? It is
  gone for good." / "Merge two Copper Garnet Rings into a Silver Garnet Ring?". Enter / Esc.
* Feedback: the god's beam (`SunBeam::PRAYER_LIGHT` tint) and a status line "Ptah accepts the offering. +40 XP" /
  "Ptah joins the rings: Silver Garnet Ring".
* Optional, the implementer's choice: the god flavours the reward (Montu favours weapons, Ptah amulets, ...: +50% XP
  for those). Leave out if it bloats the plan.
* Glossary rows: offering, merge (ring); the amulet tier row: upgraded at a shrine. Unit tests for merge rules (gold
  cannot, worn ring stays worn, needs two), amulet upgrade (not away from a shrine, yes at one; a weapon anywhere)
  and offer rules (last copy cannot, worn cannot).

## Play-test

* (play script: a shrine, two Copper Garnet Rings, a Gold ring, a level-5 weapon with spare copies, a spare amulet)
* Away from the shrine: the amulet's Upgrade is disabled, "Only at a shrine"; a weapon still upgrades.
* At the shrine, open the inventory: upgrade the amulet; merge the copper rings, see a Silver Garnet Ring; the gold ring has no Merge.
* Offer a spare weapon copy and an amulet: the confirmation shows, XP is gained, the item is gone.
* Away from the shrine: no Offer / Merge.
