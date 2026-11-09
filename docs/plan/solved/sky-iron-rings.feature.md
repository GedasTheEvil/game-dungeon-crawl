# Sky-iron rings, a tier above gold

Status: solved 2026-11-09 (play-tested by the user). Was: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size M, needs first: [resistance-tiers](../resistance-tiers.feature.md).

## Idea

A fourth ring metal above gold (`RingMetal`, today copper < silver < gold, [ring-accessories](ring-accessories.md)).
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
  ([modeling.md](../../modeling.md)) or a texture script, as the other bands were made.
* HUD ring dot setting (`player_hud.cpp`, gold setting today): fine as is, or a dark setting for sky iron.
* Lore and field note for the first sky-iron ring ("iron of heaven, fallen from Nut's body").
* Balance (user, 2026-11-09): one ring stays weaker than an amulet; two can beat one. Fix the glossary's ring row
  ("Weaker than an amulet" -> one ring is).
* Sky-iron amethyst (user, 2026-11-09): a share of 100 % on the amethyst rule of
  [resistance-tiers](../resistance-tiers.feature.md): every rate one whole tier toward more damage
  (immune -> tough -> resists -> normal -> weak -> vulnerable; vulnerable stays). The only amethyst that moves immune.
  Two sky irons: two tiers (the 200 % cap); a sky iron with a gold: 150 % (tough -> 75). Bosses get it too (it is the
  ring, not the monster). Poison resistance is not touched.


## Implementation notes

* Source: merge only (two gold rings at a shrine). No chest roll: sky iron stays the reward of merging.
* `ItemKind`: the sky-iron rings sit in each gem's row (`GarnetSkyIron` after `GarnetGold`). The ring file ids of
  copper to gold are unchanged (0-17); sky iron is `ItemType::SKY_IRON_RING_FILE_ID` + gem (18-23), so levels, saves
  and scripts keep reading.
* Garnet: `RingBonus::regenPercentPerHour` (1800 a sky-iron ring), summed with the flat rate in
  `PlayerStats::Regenerate`, max HP counted as at least 100 (the "at least 1"). Card "Heals 1% health (at least 1)
  every 2 s"; sum "heals 2% health every 2 s", with a gold one "heals 1% health every 2 s, 1 every 3.0 s".
* Amethyst: `RingBonus::amethystMovesImmune` (any sky-iron amethyst worn) lets `amethystResist` move an immune rate,
  by the whole sum: sky iron + gold on immune 37.5, two sky irons resists. Without a sky iron (two gold) immune stays.
* Lore: all six sky-iron rings share `[item.ring-sky-iron]` ("Iron of heaven, / fallen from Nut's body."). Field note
  `FieldNote::SkyIron` ([note.sky-iron]) at the first sky-iron ring, from a merge or a find. The gem notes list the
  sky-iron numbers.
* Band texture `ring_band_sky_iron` (`tools/blender/models/rings.py`, `etch_lattice`): dark iron, three sets of
  crossing etched lines in object space. HUD: a dark iron setting under the gem instead of gold.
* Offer XP 1200. Glossary rows ring (one ring weaker than an amulet), sky iron, merge.

## Play-test

* `bin/game --play tests/play/sky-iron-rings.txt`: a shrine of Isis at col 5, 20 / 50 HP.
* At the shrine, I, Rings: U on the Gold Garnet, then the Gold Amethyst: "Sky-Iron Garnet Ring" and "Sky-Iron Amethyst
  Ring", dark etched bands, short names "IV"; the journal gets the note "Sky iron".
* Wear both: the HUD gem dots sit on dark settings; health climbs 1 every 2 s.
* E at the brazier, fire arrows on the Anubis boss (col 26): fire hurts it now (immune moved to tough), it can burn.
