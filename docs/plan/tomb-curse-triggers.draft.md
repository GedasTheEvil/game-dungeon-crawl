# Tomb curses: who and what applies them

Status: draft 2026-10-10. Needs [curses](curses.draft.md) first. From the lore review, picked by the user.

## Idea

The "curse of the pharaohs": desecrating the tomb curses the player. Frames the curses as execration texts and
answers part of the curses draft's open point ("who applies which curse") for objects, beside monsters.

* Triggers (each a chance, default 25%):
  * opening a sarcophagus or a coffin (decor `sarcophagus`, `coffin`), if made interactive;
  * opening canopic jars ([treasure-containers](treasure-containers.draft.md));
  * smashing pottery or an ushabti ([living-ushabti](living-ushabti.draft.md)).
* Which curse: by the object (implementer's choice from the curses list), e.g. canopic jars → disease, a sarcophagus →
  weakness or vulnerability.
* A warning first: an inscription near the object ([hieroglyph-cartouches](hieroglyph-cartouches.draft.md) if
  present) or a status line ("A curse is written on the lid.").
* Reward for the risk: the cursed objects hold better loot.
* Journal humour: "The lid said 'cursed be he who opens this'. I opened it. The lid was right."

## Tests

* Unit / sim: a trigger rolls on the gameplay stream and applies its curse.
