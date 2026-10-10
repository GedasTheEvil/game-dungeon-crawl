# Egyptian names

Status: draft 2026-10-10, ready (needs nothing first). From the user, after a lore review: rename the monsters and
items that do not fit the Egyptian tomb. Names only (and their journal notes / flavour lines); no new behaviour.

## Rules (user, 2026-10-10)

* Item names sound Egyptian: gods, real amulet objects, real materials and peoples (Nubian, Medjay). Never made-up
  "Egyptian-sounding" words.
* Journal notes and flavour lines keep their humour (the archaeologist's dry voice). Rename the thing, keep the joke,
  or swap in a better one.
* The common Anubis becomes a living statue: **"Anubis living statue"** (or close: "Living statue of Anubis").
* Stargate jokes and references are welcome (the user loves the show), in riddles and notes alike.
* Display names only: keep `MonsterKind` ids, `ItemKind` ids and save ids, so saves still load. Renaming the code
  ids and `label`s is the implementer's choice (labels show in the level legend and checker warnings; scenarios use
  them, e.g. `tests/scenarios/anubis_bolt.txt`).

## Monsters (`src/world/monster_kinds.cpp`: `name`, `description`, `note`)

| Today | New | Note |
|---|---|---|
| Anubis | Anubis living statue | A walking statue of the god, not the god. Note e.g. "A statue of Anubis that forgot to stay on its plinth. Traps do not stop him." Blood colour: grey stone chips (implementer's choice) |
| Anubis boss | Anubis | The god himself, guardian of the last tomb |
| Boss scarab | Khepri | The scarab of the rising sun; "the mother of the scarabs" note stays |
| Vampire bat | Bat of Shezmu | Shezmu, the demon of the wine press and of blood. Note keeps "Grown fat on blood" |
| Mimic | Chest of Set | Set shut Osiris in a chest made to his measure. Note e.g. "Not every chest in this tomb holds treasure. Ask Osiris." |
| Worm | Sand worm | Dune joke welcome in the note ("Slow as sand..." can stay) |
| Man-eater plant | Thorn acacia | The acacia: Osiris's tree, thorny. Note keeps "Never stand next to it." See [plant-poison](plant-poison.md) for its new note |

The rest fit (rat, giant rat, bat, giant bat, scarab, giant scarab, mummy, crocodile, Sobek, scorpion, giant
scorpion, scorpion queen, cobra, giant cobra, Apep, egg cluster). Optional: the egg cluster note can wink at real
scorpions bearing live young.

Bosses' descriptions mention their minions by name ("summons bats"): update with the new names.

## Items (`src/world/items.cpp`: `text` name, short name, label, flavour lines)

Short names must still fit the inventory and HUD (see the current lengths). Keep the effect text.

### Weapons

| Today | New |
|---|---|
| Club | Acacia Club |
| Dagger | Flint Knife |
| Short Sword | Bronze Sword |
| Khopesh, Epsilon Axe, Duckbill Axe, Throwing Stick | keep (already Egyptian) |
| Mace | Pear Mace (the pear-shaped head of Narmer's mace) |
| Spear | Medjay Spear |
| Self-Bow | Nubian Bow (Ta-Seti, "the land of the bow") |
| Composite Bow | Chariot Bow |
| Sling | Nubian Sling |
| Javelin | Medjay Javelin |

"Now with spikes." and "None shall pass!" may stay (humour) or get a better line.

### Potions

| Today | New |
|---|---|
| Small Health / Large Health | Lesser / Greater Draught of Sekhmet (patron of healers) |
| Aphethamine | Blood of Montu (war god); keep "It tingles. Best not ask what is in it." |
| Stone Skin | Granite of Ptah |
| Elixir of Life | Breath of Osiris |
| Small Stamina / Large Stamina | Date Wine / Beer of Hathor (the red beer that calmed Sekhmet) |
| Antidote | Milk of Renenutet |
| Lesser / Greater Resistance | Lesser / Greater Ward of Wadjet |

### Amulets (`AMULETS`, "Amulet of <noun>")

Name each by the object its flavour line already describes. The tier words stay (Lesser, Minor, Grand); the format
("Lesser Jackal Tooth" vs "Lesser Amulet of the Jackal Tooth") is the implementer's choice, as long as the effect
line shows what it does.

| Today | New noun |
|---|---|
| Strength | Jackal Tooth |
| Armor | Bronze Scarab |
| Health | Carnelian Heart (ib) |
| Poison Warding | Serket's Scorpion |
| Trap Warding | Eye of Horus |
| Blunt Warding | Djed Pillar |
| Slash Warding | Knot of Isis |
| Pierce Warding | Shen Ring |
| Regeneration | Faience Lotus |
| Venom | Wadjet's Cobra |

## Riddles

* Add a Stargate riddle file (`riddles/stargate.txt`, theme e.g. "Chevron Seven Locked"), 10+ riddles: Ra, Apophis,
  Jaffa, Kree, chevrons, Abydos, Daniel Jackson, "Indeed.", the Goa'uld, the DHD, naquadah. Plain ASCII, answers
  per `docs/riddles.md`. The two in `riddles/scifi.txt` stay where they are, or move to the new file.
* The other riddle files stay.

## Also update

* Journal field notes and riddle hints that name a renamed thing.
* `docs/glossary.md` (e.g. the kin row: "the Anubis boss: the Anubis guard"), `docs/levels.md` legend if it shows
  names, and drafts that use the old names: [cursed-mimic](cursed-mimic.draft.md) (becomes a cursed Chest of Set),
  [curses](curses.draft.md), [anubis-ranged-attack](anubis-ranged-attack.draft.md),
  [monster-balance](monster-balance.draft.md).
* Unit tests and scenarios that check names.

## Tests

* `make unit` (docs_test checks glossary code names), `make test`, `./levelcheck levels/lvl*` (no warnings).
* Screenshot check of the inventory (long names fit) and a journal page.
