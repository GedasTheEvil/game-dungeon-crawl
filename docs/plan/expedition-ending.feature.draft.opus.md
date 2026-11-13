# The earlier expedition's end: D. J.'s remains, and the epilogue answers his last page

Status: draft 2026-11-13, size M, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The game's second story line, the earlier expedition (D. J., the linguist), is set up across 24 pages and never closed.

* Pages by zone (`content/texts/expedition.txt`, the Page objects, type 20, in `content/levels/lvl*`): zone 1: 3,
  zone 2: 4, zone 3: 2, zone 4: 2, zone 5: 2, zone 6: 3, zone 7: 1, zone 8: 2, zone 9: 2, zone 10: 1, zone 11: 2
  (pages 23, 24 on lvl46, lvl49), zone 12: 0. D. J.'s trail stops at the last fork (lvl49), which fits: he died there.
* Page 24 ("The last page", on the way to lvl49's fork): "take the left gate. I took the right. Behind it, the tomb of
  the one who came before me, and worse. The ankh is real. Tell them I was right."
* His remains exist: on lvl49's right route ([denser-levels](solved/denser-levels.md#route-fork-left-or-right),
  `placeForkContent`, `src/level/level_gen_content.cpp`) a skeleton prop at (34,9) and his gear chest at (35,9)
  (decor and objects lists of `content/levels/lvl49`; y counts from the bottom). Nothing there says it is him: no text,
  no page. The plan promised "their journal's end" there; the page went before the fork instead.
* A player who obeys the page and goes left never learns anything more. Inscription 136 (The last gate, lvl54)
  guesses: "He is not here. Maybe he took the right gate. Maybe he was right." Its comment: "I will find out. Then I
  will say it for the page."
* The epilogue (`[diary.dawn]`, `content/texts/journal.txt`) never mentions him. "Tell them I was right" and "I will say
  it for the page" are not paid off.
* Also open from the set-up: page 4's "Someone planned this" and the prologue's "Someone built this maze to keep it. Or
  to keep someone in." Nothing answers who; and page 24's "the one who came before me" is nobody.

## Plan

1. **His notebook's back cover.** A last notebook text at the remains on lvl49's right route: either page 25
   (`EXPEDITION_PAGE_COUNT` 24 -> 25, `src/level/level.h`; a Page object on the skeleton's cell, `tools/level/lvledit.py`;
   the expedition unit test numbers the pages) with heading "Inside the back cover", or a field note written when the
   gear chest is opened. Only a right-route player finds it. Sample, his voice (short, dry, frightened):
   "Day 41. Right gate. Traps on every other stone, and at the end a skeleton older than the tomb, with a notebook in a
   hand I cannot read and a jackal drawn on every page. So I was not the first, and he was not either. The jackal does
   not want the ankh kept. He wants it carried. I cannot walk any more. The left gate. Tell them."
   Mind the order problem: page 24 says "I took the right" yet lies before the fork. Either the back cover is written
   first and page 24 is his note thrown back up the shaft (say so in page 24 or 25), or leave page 24 as is and let the
   back cover be after it in time. Pick one and keep the two pages consistent.
2. **The epilogue answers him.** A fourth epilogue paragraph or a second diary page after Dawn (`DiaryPage`,
   `src/level/journal.h`, written in `src/ui/win_entry.cpp` beside `WriteDiary(DiaryPage::Dawn)`), shown only if page 24
   was found (the journal knows the pages read). Sample: "In my pocket, a torn page in another man's hand. Take the left
   gate, it says; tell them I was right. D. J., a linguist, not a soldier. I took the left gate. You were right." A
   variant if the back cover (step 1) was read too.
3. **Lvl54's guess.** Inscription 136's last line and comment can stay (the player's guess on the way), or say what the
   right-route player knows. Keep it working for both routes.
4. **Who built it (one answer, not a lore dump).** One sentence in the back cover or the epilogue: the maze is the
   road of Ra's night, and the jackal statue (Anubis, the prologue) lets the living walk it to carry the ankh out; each
   one who failed is a skeleton under a page. This is what the existing hints already imply (the jackal "has always been
   waiting", id 128; pages 1 and 23).

Docs: `docs/levels.md` (the expedition pages paragraph), the glossary row for the expedition if the page count or a
new diary page changes. `bin/levelcheck content/levels/lvl49` stays warning-free (a page in reach).

## Open questions

* Page 25 (a Page object, counted in the Expedition ribbon) or a field note on the chest? A page keeps the pages in one
  place; it breaks "the last page" being the last (rename its heading?).
* Should D. J. get a full name? The texts keep the nod light ("D. J.", "Who is Jack?", "Daniel would know").

## Play-test

* `bin/game --level 49`: take the right gate, reach the skeleton at the route's end, read the back cover in the journal
  (J, Expedition).
* A play script by the ankh with page 24 in the journal: take the ankh, the epilogue answers D. J.
