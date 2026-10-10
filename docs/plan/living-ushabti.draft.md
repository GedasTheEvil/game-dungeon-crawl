# Living ushabti

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

Ushabti are the small servant figures buried to work for the dead. The `ushabti` decor (`DECOR_NAMES`,
`src/world/decor.h`) comes alive: a new monster that looks like the decor until the player comes near, then stands
up and walks. Same family as the Anubis living statue ([egyptian-names](egyptian-names.draft.md)).

* Kind: small, slow walker, a few HP. Glazed faience shatters: **weak to blunt**, resists pierce. Journal note with humour: "It says 'here I am' on its chest. It means it."
  (the ushabti spell, Book of the Dead ch. 6).
* Lurk: idle as the decor figure (like the mimic's ambush), wakes in range.
* Placement: among real ushabti decor, so the player cannot tell; levels where ushabti decor appears.
* Anubis boss: summons ushabti instead of (or beside) mummies (`Summon::Coffin` → a new rise from the floor, or the
  existing one): implementer's choice.
* Glyph: pick a free one. Model: the ushabti decor model, rigged with a walk, rise and death (crumbles) clip.
* No boss has it as kin unless the Anubis boss's kin changes; then the boss rule applies.

## Tests

* Unit / sim: wakes in range, walks, takes double blunt.
* Scenario with a screenshot: idle among decor ushabti, awake.
