# Giant cobra: blinding spit

Status: draft 2026-10-19, ready (needs nothing first). From the user's review: each giant gets an ability of its
own. Today the giant cobra spits as the cobra does, only farther (plan giant-cobra).

## Idea

The giant cobra aims its spit at the eyes. A hit blinds the player for a short time, beside the poison.

* **Blind:** ~2 s: the screen edges go dark and blurred, only a small clear spot in the middle; the view clears
  over the last 0.5 s. Aiming the bow and seeing what comes is hard. Movement and attacks work.
* Only the spit blinds, at 50% of spit hits (gameplay stream); the bite does not.
* Water helps: wading in half water or in water clears it twice as fast (wash the eyes).
* Look and the effect: a post-process like the sprint vignette ([sprint-motion-effect](solved/sprint-motion-effect.md)),
  green tinted. Toon mode too.
* Journal note: "Black cobras aim for the eyes. Stand in the water and blink."

## Boss: Apep

Boss rule (his `kin` is the cobra): his spit is a fan of three, and each hit blinds ~3 s.

## Tests

* Unit / sim: the roll, the blind's time, water halves it, Apep's fan of three.
* Scenario with screenshots: blinded view, normal and toon mode.
