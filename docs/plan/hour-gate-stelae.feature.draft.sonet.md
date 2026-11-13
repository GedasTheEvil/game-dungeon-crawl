# Hour stelae: what the Amduat puts in each hour, one stela per zone

Status: draft 2026-11-13, size M, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

Every zone is an hour of Ra's night journey, with a gate (`zoneTitle`: "The Fourth Hour: the Gate of the Priests",
`ZONE_INFO` in `src/level/campaign.h`). The only thread through the hours is the twelve hour field notes
(`[note.hour-1]`..`[note.hour-12]`, `content/texts/journal.txt`). On the walls the hours are nearly absent:

* Of 136 inscription texts, three speak of an hour of the night: id 91 (Mehen in the seventh hour, lvl26), id 121 (the
  Book of Gates lake, lvl44), id 8 ("Every hour of the night has its gate, every gate its keeper", lvl38). No keeper is
  ever named. (`grep -n -i "hour\|boat\|barque\|Duat" content/inscriptions/*.txt | grep T:`.)
* What a player who knows the Amduat expects and never meets (counted over `content/inscriptions/*.txt` and
  `content/texts/*.txt`, case-insensitive):

  | Figure or idea | Hits | Its hour |
  |---|---|---|
  | Ra as the ram-headed night sun in his shrine on the barque ("Flesh", iwf) | 0 | all, from hour 1 |
  | the baboons who greet Ra and open the doors | 0 for the Duat (baboons only as Babi's and Thoth's) | 1 and 12 |
  | Aker, the double lion who guards Sokar's cavern | 0 | 5 |
  | Nephthys (beside Isis in the boat and at the end) | 0 | 7, 12 |
  | Atum | 0 | 11 |
  | the pits of fire where Ra's enemies burn | 0 | 11 |
  | the hour goddesses who guide the boat | 0 | each hour |
  | the drowned rescued by Horus | 1 (the hour-10 note) | 10 |
  | the ba (the bird-soul that rejoins its body) | 0 | 6 |
  | Nut, who swallows the sun at evening and bears it at dawn | 3, none about that | 12 |

* The game's sphinx statues hold the portals between levels, and the riddle gates are sphinxes: Aker (double sphinx,
  guardian of the earth's gates) is the obvious tie, and no text makes it.

## Plan

Twelve stela inscriptions, one per hour, placed on each zone's first level (lvl1, 5, 10, 14, 19, 23, 28, 33, 38, 42, 46,
51), near the entrance, so the zone's hour is announced on the wall as the banner announces it on screen. Each names
the hour's goddess (the guide of the boat in that hour) and one or two figures of that hour, and ends in a short
line that suits the zone's play. Ids: the next free ones in `content/inscriptions/campaign.txt`; style `stela`; no
boons (the zone-first levels already hold their level's boon where they have one, check). Format and the sheet rebuild
for new signs: [inscriptions.md](../inscriptions.md). Level placement: `tools/level/lvledit.py LEVEL wall COL ROW 15 ID 0`
on a back wall the player can stand before; `bin/levelcheck content/levels/lvl*` stays warning-free; update the
level table rows in `docs/levels.md`.

The hour goddesses (after Hornung's Amduat; verify each against a translation before use, the accuracy audit runs
in parallel): 1 "she who smashes the foreheads of Ra's enemies", 2 "the wise one who protects her lord", 3 "she who
cuts the bas", 4 "she who is great in her power", 5 "she who guides in the midst of her boat", 6 "she who arrives, the
right guide", 7 "she who repels the serpent and beheads Hostile-face", 8 "lady of the deep night", 9 "she who adores
and protects her lord", 10 "the raging one who slaughters the rebel", 11 "the starry one, lady of the boat", 12 "she
who sees the beauty of Ra".

Content per hour (one or two of these, the rest stays out):

| Zone (lvl) | Hour | What to bring in |
|---|---|---|
| Tomb entrance (1) | 1 | the boat enters the west; Ra ram-headed in his shrine on it; baboons sing and open the doors |
| Bat galleries (5) | 2 | Wernes, the fields of grain by the water; the gods of the year |
| Web halls (10) | 3 | the waters of Osiris; the boat rows on in the dark |
| Priests' quarter (14) | 4 | Rosetau, the sand road; the boat turns into a serpent and is towed |
| Scorpion nest (19) | 5 | Sokar's cavern; Aker the double lion guards it (the sphinx gates' ancestor); Isis's head over the mound |
| Serpent temple (23) | 6 | midnight: the ba of Ra joins his body, Osiris; the serpent around the corpse |
| Cisterns of Sobek (28) | 7 | Apep on his sandbank, knives in his coils; Isis and Nephthys's spells; Mehen |
| Seth's desert (33) | 8 | the dead clothed; the caverns that answer the sun's voice |
| Land of Sokar (38) | 9 | the rowers of the boat; the dead fed (note: the zone is named for Sokar, who belongs to hours 4-5; the stela can say "Sokar's land lay hours back", as the hour-9 note does) |
| Lake of Fire (42) | 10 | the drowned pulled from the water by Horus; the archers of the boat |
| Necropolis (46) | 11 | the pits of fire where Ra's enemies burn; Atum; the serpent that carries the sun's disc |
| Hall of Judgment (51) | 12 | the serpent Life of the Gods; Ra comes out as Khepri; Shu and Nut receive him |

Sample (hour 1), to tune:

  title: The first hour
  T: The boat of Ra enters the west, and the doors of the earth open.
  T: Ra stands in his shrine with the head of a ram, the flesh of the god.
  T: The baboons sing to him and open the gates of the hour.
  T: She who smashes the foreheads of his enemies goes before him.
  N: A ram-headed sun, singing baboons. The first page of a very long book.

Sample (hour 5):

  title: Aker at the cavern of Sokar
  T: Aker, the two lions back to back, keeps the cavern of the fifth hour.
  T: He lets the boat pass and closes behind it.
  T: The sphinx at every gate is his: answer, and it opens.
  N: So that is why the gates are sphinxes. They are on night shift.

## Open questions

* A fourth inscription on some zone-first levels (lvl14 has five texts with its cartouche). If that is too many, replace a
  generic one there, not a boon or a hint.
* The hour notes stay as they are; if a stela and its hour note say the same thing, make the note the archaeologist's
  reaction and the stela the fact.

## Play-test

* `bin/game --level 1`, `--level 19`: read the hour stela by the entrance (Words of Thoth needed: a play script that
  sets the skill), the journal page under Inscriptions.
