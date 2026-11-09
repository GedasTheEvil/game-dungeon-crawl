# Sky-iron rings, a tier above gold

Status: draft 2026-11-09, size M, needs first: [resistance-tiers](resistance-tiers.feature.md).

## Idea

A fourth ring metal above gold (`RingMetal`, today copper < silver < gold, [ring-accessories](solved/ring-accessories.md)).
Garnet example from the user: heals `max(1, 1% of max HP)` every 2 s, the fastest regeneration in the game. The other
gems get a boost of a similar step.

Metal: **meteoric iron**, "sky iron" (Egyptian *bia-n-pet*, "iron of heaven"). Why it fits:

* Rarer than gold in pharaonic Egypt: only from fallen stars (Tutankhamun's iron dagger and amulets are meteoric).
* Myth: the gods' bones are silver, their flesh gold (Book of the Heavenly Cow), the sky's vault is iron; the Opening
  of the Mouth adze is of sky iron. Fits the Duat, Nut's night sky (lapis) and the Stargate.
* Looks unlike the other three: dark grey with a faint etched crystal pattern (Widmanstätten), the gem stands out.

Rejected: electrum (gold-silver alloy, prized for obelisk tips, but pale gold, hard to tell from gold on the HUD and
in the world; also odd as "above gold" when it is half silver); orichalcum (Greek, Atlantis).

Name in texts (user agreed; "divine iron" the alternative): "Sky-Iron Garnet Ring" (the item name pattern "Gold Garnet Ring"). Glossary row.

## Plan

* `RingMetal::SkyIron`, `RING_METAL_COUNT` 4. New `ItemKind`s per gem (6), `ringOf` / `ringKind` keep working.
  Saves store item numbers: the inserted kinds shift later ones; old saves may be ignored (AGENTS.md).
* Source: merge only, two gold rings at a shrine (`nextMetal`: gold -> sky iron, sky iron the top). Maybe also a rare
  chest roll deep in the campaign (from level 40, 3 %): implementer's choice, say so in the plan.
* Effects (fourth column of `RingEffect`), a step like silver -> gold:
  | Gem | Copper / Silver / Gold | Sky iron |
  |---|---|---|
  | Garnet | 1 HP per 8 / 5 / 3 s | `max(1, 1% max HP)` per 2 s |
  | Lapis | 15 / 25 / 40 % | 60 % |
  | Feldspar | 20 / 35 / 50 % | 70 % |
  | Jasper | 5 / 8 / 12 % | 17 %, `JASPER_CAP` 20 -> 25 |
  | Crystal | 10 / 15 / 25 % | 35 %, `CRYSTAL_CAP` 40 -> 50 |
  | Amethyst | 10 / 20 / 50 % of a tier (resistance-tiers) | a whole tier (below) |
* Garnet: `RingBonus::regenHpPerHour` is a flat rate; the percent of max HP needs a second field (e.g. `regenMaxHpPermille`
  per hour) summed with the flat one in `PlayerStats`; still not while poisoned. Two sky-iron garnets: twice.
  The ring card text: "Heals 1% health (at least 1) every 2 s".
* Band texture `ringBandTexture(SkyIron)`: dark grey iron, the etched crystal lattice, a cold sheen. Blender 5
  ([modeling.md](../modeling.md)) or a texture script, as the other bands were made.
* HUD ring dot setting (`player_hud.cpp`, gold setting today): fine as is, or a dark setting for sky iron.
* Lore and field note for the first sky-iron ring ("iron of heaven, fallen from Nut's body").
* Balance (user, 2026-11-09): one ring stays weaker than an amulet; two can beat one. Fix the glossary's ring row
  ("Weaker than an amulet" -> one ring is).
* Sky-iron amethyst (user, 2026-11-09): a share of 100 % on the amethyst rule of
  [resistance-tiers](resistance-tiers.feature.md): every rate one whole tier toward more damage
  (immune -> tough -> resists -> normal -> weak -> vulnerable; vulnerable stays). The only amethyst that moves immune.
  Two sky irons: two tiers (the 200 % cap); a sky iron with a gold: 150 % (tough -> 75). Bosses get it too (it is the
  ring, not the monster). Poison resistance is not touched.


## Play-test

* 
