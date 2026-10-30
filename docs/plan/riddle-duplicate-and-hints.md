# Riddles: duplicate and hints

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

* `content/riddles/scifi.txt` and `content/riddles/stargate.txt` both ask "In Stargate, which Egyptian sun god..."
  (answer ra / re). Remove the copy from `scifi.txt`.
* `scifi.txt` has no hints in 23 riddles, `pop.txt` 1 in 18. Without a hint the game shows only the answer length
  after two wrong tries. Write a hint for each (format: [riddles.md](../riddles.md)).
* A unit test (or a levelcheck-style check) that no two riddles share a question.
