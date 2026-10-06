# More Egyptian weapons

Status: draft 2026-10-06. Depends on [inventory-overhaul.md](inventory-overhaul.md): the weapons group
passes the 8 slots of one tab page.

## Idea

Ancient Egypt had many weapons. Put all of them from
[screenshots/Egyptian-Weapons-Summary-Version-2.jpg](screenshots/Egyptian-Weapons-Summary-Version-2.jpg) into the game,
except the war chariot.

| Weapon | In the game | Kind |
|---|---|---|
| Spear | yes | melee, long reach |
| Self-bow | bow (yes) | ranged |
| Composite bow | new | ranged, stronger bow |
| Javelin | new | thrown |
| Throwing stick | new | thrown |
| Sling | new | ranged, stones |
| Epsilon axe | new | melee |
| Duckbill axe | new | melee |
| Mace | new (club stays?) | melee, blunt |
| Dagger | new | melee, short |
| Khopesh | new | melee, slash |
| Short sword | sword (yes)? | melee |

Each needs a model built in Blender ([../remodeling.md](../remodeling.md)), an `ITEM_DEFS` row with its damage mix
([damage-types-and-resistances.md](solved/damage-types-and-resistances.md)), reach and speed, and a place in the
campaign's chests.

## Open

* The existing club, sword and bow: kept beside the new ones, or become the mace, short sword and self-bow?
* Thrown weapons (javelin, throwing stick): ammo like arrows, or picked up again after a throw? The throwing stick
  coming back?
* Sling ammo: stones, found or endless.
* Each weapon's role, so they are not 12 versions of one: damage type, reach, speed, stamina cost
  ([monster-balance.draft.md](monster-balance.draft.md)).
* Two bows: what the composite bow adds over the self-bow (range, damage, draw time).
* In-game hotkeys: four weapon keys today; which weapons get one.
* Save games and level files name a weapon by type and id (`ItemType`, `src/world/items.h`); new ids at the end.
