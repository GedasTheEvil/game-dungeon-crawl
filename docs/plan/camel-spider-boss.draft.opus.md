# Camel spider boss: the web spinner

Status: draft 2026-10-20, from the user ("for the remaining bosses, a spider boss makes sense; others remain without
a boss"). Needs nothing first. Placement: open (see below).

## Lore

Egyptian myth has no spider god, and spiders are rare in Egyptian art (Neith is the goddess of weaving, but she is
never a spider). So this boss is a beast, not a god, like the hippo before Seth. The hook comes from the game itself:
the camel spider's journal note ends with "It spins no webs. So who made these?" The decor webs (`DECOR_WEB`) in the
halls are hers. Name: implementer's choice (for example "the Mother of Webs"); not a god's name.

## Idea

`MonsterSpiderQueen` (working name), kin `MonsterCamelSpider`. Boss rule (AGENTS.md): the camel spider's abilities,
stronger.

* Model: the camel spider's, scaled up (about 2.5x), own texture (darker, banded, a pale sheen), or her own model if
  she should look like a web spinner (a rounder body, spinnerets). Implementer's choice.
* **Shuns the light, stronger:** faster than the camel spider (which already outruns the player). The camel spider
  only keeps out of the light; she puts it out: a torch or a lamp near her goes dark when she passes (darkness spreads
  with her), so the light the player relies on does not last. The braziers stay lit (the player's refuge, and the fire
  arrows' source).
* **Webs:** she spits or lays web on the floor of her row: a cell of web holds the player like the sand fall's slow
  (or a short hold, `HoldRules`); a sword cuts it, fire burns it. The decor webs are hers.
* Minions: camel spiders drop from the ceiling (`Summon::Drop`, as the vampire bat's bats), out of the webs.
* Weakness: the camel spider's (blade, fire); `bossResist` drops the weaknesses to normal.
* Journal note answering the camel spider's: "Found the one who makes the webs."

## Placement (open)

Final place (user, 2026-10-21): the web halls, zone 3 of [campaign-zones](solved/campaign-zones.md) (about
levels 10-13), after the rebuild. Until then, in today's campaign:

* The desert (21-25), where the camel spiders run: as a mid-chapter boss off the every-fifth-level rhythm, or in an
  extra level.
* An early level near the camel spiders' first levels (4, 6-8), as a minor boss.
* No campaign slot, play script only, as hippo-boss did at first.

## Others

The rest stay without a boss (user, 2026-10-20): rats, plants, mimics, the lotus. The worm has its own draft
([worm-boss](worm-boss.draft.opus.md)).

## Open (warn when reporting)

* The placement.
* Putting out the torches: every torch she passes, or only near the player; whether they relight.
* Webs as a slow or a hold.
* Her own model or the camel spider's, scaled up.

## Tests

* Unit / sim: faster than the camel spider, keeps out of the braziers' light, puts out a torch, the web slows or holds
  and a blade or fire clears it, camel spiders drop round her.
* Scenario with a screenshot: in the dark, a torch going out, a web on the floor.
