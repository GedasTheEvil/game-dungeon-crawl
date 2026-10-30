# Lore corrections

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Text-only fixes against Egyptian sources. Keep the notebook voice ([texts.md](../texts.md)); change the fact, not the
style. Grep every name to catch all copies (readme, `content/`, `src/` strings, docs).

## Factual errors

1. **Khepri is male.** "The mother of the scarabs" (readme "A boss for every zone", `content/texts/monsters.txt`
   boss-scarab note, `moves.txt` roll-ball "She..."). Egyptians held that all scarabs were male and self-created.
   Make him "lord of the scarabs", "he". Text only; the boss itself is out of scope.
2. **Hour notes** (`content/texts/journal.txt`, `note.hour-N`) vs the Amduat:
   * Hour 3 "the judging of Osiris": the Amduat 3rd hour is the Waters of Osiris. Judgment belongs to the Book of
     Gates between hours 5 and 6.
   * Hour 7 "Apep waits here": Apep is already killed in zone 6. Move the Apep line to the hour-6 note. In hour 7 Apep
     lies on a sandbank, having drunk the river; Isis and Seth's magic and Serket bind him; Mehen coils round Ra.
   * Hour 9 "land of Sokar": Sokar's land (Rosetau) is hours 4-5.
   * Hour 10 "the lake is cooled": hour 10 is water and the rescue of the drowned. Fiery lakes are in hour 5, the
     fire pits of punishment in hour 11.
   * Hour 12 "hall of the scales": hour 12 is rebirth (Ra through the serpent, out as Khepri at dawn).
   Check the zone themes still read right after the change (`src/world/campaign.h`, `zone_look.h`).
3. **Inscriptions** (`content/inscriptions/rebuild.txt` and the matching `campaign.txt` entries):
   * id 121 "Book of Gates, the tenth hour... lake of flame": the Lake of Fire is in hour 3.
   * id 116 "Two baboons guard the shore": four baboons, at the corners (Book of the Dead spell 126).
   * id 129 "Seventy days in natron, and seventy days wrapped": 70 days in all, about 40 in natron.
4. **Babi "eater of hearts"** (`src/world/zone_look.h`, `monsters.txt` babi): that is Ammit. Spell 125: Babi "lives on
   the entrails". Use "eater of entrails" or "bull of the baboons".
5. **Apep "who swallows the sun each night"** (`monsters.txt` apep): "who would swallow the sun"; he fails every night.
6. **King notes** (`content/names/kings.txt`):
   * Nectanebo: spelling "nakhtnebef" and signs are Nectanebo I; "the last native pharaoh, after him the Persians" is
     Nectanebo II (Nakhthorheb). Make them agree.
   * Akhenaten "one very short dynasty": the 18th Dynasty lasted ~250 years; "one very short revolution".
   * Merenre "Harkhuf came back with a dancing pygmy": that was under Pepi II (his letter). Move or reword.
   * Seti "the longest tomb in the valley": KV17 is the longest *decorated* one (KV20 is longer).
7. **Camel spider legs**: `monsters.txt` "Ten legs" vs expedition page 7 "eight legs". Eight legs and two leg-like palps.

## Consistency

* Serqet vs Serket (`campaign.h`, `zone_look.h`, `journal.txt`, `rebuild.txt`): use Serket everywhere.
* "Chest of Set" / "Cursed chest of Set" (`src/world/monster_kinds.cpp`) vs Seth elsewhere: use Seth.
* `docs/levels.md` says the zones follow the Book of Gates while the lore cites the Amduat: say it is a blend
  (Amduat hours, Book of Gates style gate titles).

## Debatable (fix if cheap)

* Sobek "lord of the Nile": Hapi is the Nile flood; Sobek is "lord of the waters", "of Shedet".
* "Breath of Osiris" (`src/world/items.cpp`): the breath of life comes from Amun / Ra or the north wind.
* "Seth's Mace": Seth's own things are the was-sceptre and iron ("bones of Seth").
* "Jackal Tooth": not an Egyptian amulet; the ox-foreleg or a Montu amulet would be authentic.
* Seth spearing Apep in the prow (hour-8 note): Seth's magic in the prow belongs to hour 7; the spear is later.
* Wenamun "nineteen days" (`rebuild.txt`): Lichtheim reads 29.
* Aswan granite "cut with fire and ram": pounded with dolerite balls.
* id 49 copper harpoon: in the Contendings Isis makes and throws it (the Edfu version, Horus, is fine).
* "Hunro" as a man (`rebuild.txt`): Hunro is a woman's name at Deir el-Medina (low confidence; verify).

Run the text unit tests and `bin/levelcheck content/levels/lvl*` (inscriptions are placed by id).
