# Camel spider

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

The camel spider (solifuge) of the desert: fast, big jaws, the stuff of soldiers' stories. It also gives the
`web` decor a joke, because camel spiders spin no webs.

* Kind: a fast walker, low HP, slash bite. Folklore: it chases your shadow. Mechanic: it runs at the player in the
  dark and **flees from light** (torches, braziers nearby): it will not step into a lit cell's range
  (implementer's choice how light is measured; braziers and wall torches as sources).
* Not a poisoner (real ones are not venomous): journal note says so, with humour: "Not venomous, they say. It does
  not need to be."; and on the webs: "It spins no webs. So who made these?"
* Levels: early-middle desert-feel levels (sand floors).
* Boss: none has it as kin.
* Glyph: pick a free one. Model: procedural (8 legs plus long pedipalps, so it looks ten-legged), clips: walk, bite, death.

## Tests

* Unit / sim: chases the player, stops at a light source's range.
* Scenario with a screenshot.
