# Tomb curses: who and what applies them

Status: draft 2026-10-10. Ready: [curses](solved/curses.md) is in (2026-10-12). From the lore review, picked by the user.

## Idea

The "curse of the pharaohs": desecrating the tomb curses the player. Frames the curses as execration texts and
answers part of the curses draft's open point ("who applies which curse") for objects, beside monsters.

* The rule (one roll per object, the tell, the trap-ward amulet, Isis, the loot) is
  [trapped-chests](trapped-chests.draft.opus.md#curses-from-objects-one-rule)'s; this plan adds the objects that are
  not treasure. Canopic jars are a container: their curse is a chest trap there, not a trigger here.
* Triggers (each a chance, default 25%):
  * opening a sarcophagus or a coffin (decor `sarcophagus`, `coffin`), if made interactive;
  * smashing pottery or an ushabti ([living-ushabti](living-ushabti.draft.opus.md)).
* Which curse: by the object (implementer's choice from the curses list), e.g. a sarcophagus →
  weakness or vulnerability, pottery → disease.
* The tell: an execration text on the object, seen up close (no status line, no prompt text: the shared rule). With
  the [hieroglyph skill](skill-hieroglyphs.md) it may read as a warning when looked at; implementer's
  choice.
* Reward for the risk: an object that holds loot follows trapped-chests' contents rule.
* Journal humour: "The lid said 'cursed be he who opens this'. I opened it. The lid was right."

## Tests

* Unit / sim: a trigger rolls on the gameplay stream and applies its curse; the trap-ward amulet's chance stops it.
