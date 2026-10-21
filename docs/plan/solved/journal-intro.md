# Journal intro: the story starts in the notebook

Status: draft 2026-10-19, ready (needs nothing first). From the user. The game has no intro today: a new game drops
the player into lvl1 without a word.

## Idea

A new game opens the journal on a first entry, the archaeologist's own words: how they got here, what they found,
what they hope to find. The player reads it, closes the journal and stands in lvl1. The story's frame for the whole
campaign: the ankh in lvl35 is the secret of eternal life the entry speaks of.

* When: `GameState::NewGame` only, after lvl1 is loaded, before the first frame of play. Not on load, not on a
  restart after death (implementer's choice: also after death, if it reads as "back at the start").
* Where: a new journal section, **Diary** (the archaeologist's own entries), first in the ribbon order. Today's
  sections are Creatures, Riddles, Field notes, Inscriptions (`JournalView::Section`). The prologue is its first
  entry; later plans can add more (an epilogue on the win, a line per boss). It stays readable in the journal.
  The other expedition's pages ([expedition-notes](expedition-notes.md)) are someone else's notebook,
  not this section.
* Look: the journal as it is ([ui.md](../../ui.md), the handwritten journal font). Opens with the page-turn sound, no
  status line. A hint at the bottom of the page: the key that closes the journal.
* Game time does not run while it is open (as the journal today).

## Text (default, the implementer may polish it)

Two pages at most. No gender for the archaeologist.

> **Saqqara, the third week.**
> The dig gave us nothing for nineteen days. On the twentieth, under the sand of the east trench, a statue no
> catalogue knows: a jackal-headed man, life-sized, an ankh in his hand, and no name on the base. The crew would not
> go near it. I am a scientist. I touched the ankh.
>
> Light. Then cold stone and darkness. No sand, no sky, no crew. One torch burns on the wall, as if someone expected
> me.
>
> **Later.**
> At my feet lay an old acacia club. Not a trowel, but it will do: something scratches in the dark down the hall.
> The corridor slopes down. There is no way back that I can see, so down it is.
>
> The first signs I can read speak of one thing only: the ankh, life without end. Someone built this maze to keep
> it. Or to keep someone in. If there is a way out, it is at the bottom, with the ankh.
>
> I will keep notes. Someone should know what happened here.

The jackal-headed statue foreshadows the Anubis boss of lvl35; the club is the starting weapon (`items.cpp`).

## Fits with

* [duat-hours](../campaign-zones.draft.opus.md#the-twelve-hours-of-the-duat) (merged into campaign-zones): level titles in the archaeologist's voice, the Duat as what
  the archaeologist learns the maze to be, not a second frame.
* [expedition-notes](expedition-notes.md): the earlier expedition came through the same statue.

## Tests

* Unit / sim: a new game marks the prologue as written and requests the journal on the Diary section; a loaded game
  does not.
* Scenario with a screenshot: a new game's first frame, the open prologue page.
