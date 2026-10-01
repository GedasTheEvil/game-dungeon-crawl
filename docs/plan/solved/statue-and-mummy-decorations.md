# More corridor decorations: broken statues, mummy by a sarcophagus

Status: done (2026-09-30). Props `cat`, `jackal`, `osiris`, `bes`, `sarcophagus` in `decor.py` / `decor.h`; in-game check
`tests/scenarios/statues.txt`. Deities: Bastet (cat), Anubis (jackal on a shrine), Osiris, Bes (household god, instead of
Ra: a distinct squat silhouette, and the falcon head is already on a canopic lid). The sarcophagus fits the normal prop
limits (no exception needed).

## What

New static corridor props:

* **Broken statues**: limestone cats (Bastet style, seated) and deities (for example Anubis, Horus, Thoth). Missing
  heads, snapped ears or arms, cracks, chips on the floor next to them.
* **Mummy by a broken sarcophagus**: the sarcophagus of a common person, not a king. A plain wooden or clay coffin,
  simply painted, with the lid cracked or shoved aside. The mummy lies or slumps next to it, its wrappings torn.

## Where it fits

* Models: `tools/blender/models/decor.py` (`PROPS` list, one `build_<name>` each, output `models/decorations/decor_<name>.md3`).
  See [../../remodeling.md](../../remodeling.md).
* Engine: `src/world/decor.h` (`DECOR_COUNT`, `DECOR_NAMES`); placement in `src/world/dungeon_decor.cpp`.
* Size limits from `decor.py`: about 0.5 tile tall, within x = +-0.42 and y >= -0.43, so the props stay clear of the
  player's walk line. The sarcophagus is long, so it may need its own size or a place along the back wall.
* Check: `tests/scenarios/props.txt` screenshots.

## Decisions

* Statues: 1-4 separate props (for example a cat and one to three deities), not one model with variants.
* The mummy here is a decoration only. A mummy monster is a separate idea:
  [mummy-minion-monster.md](mummy-minion-monster.md).
