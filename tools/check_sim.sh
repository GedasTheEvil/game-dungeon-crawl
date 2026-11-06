#!/usr/bin/env bash
# Checks that the sim (src/sim/: the dungeon, the monsters, the player) stays off the app (plan world-without-game):
# no Game(), no game_state.h, no screens (ui/) or input. What it needs comes in by SimLinks / MonsterLinks or a
# parameter, and what it tells the app goes out by WorldEvents. It draws nothing: no GL, no graphics/ or render/
# header; the dungeon draws itself in src/graphics/dungeon_render*.cpp, in the game (plans sim-unit-tests,
# src-dirs-by-layer). make layers checks the includes too; this names the reason.
#
#   tools/check_sim.sh FILES...
#
# Prints one line per violation and exits 1 if there is any.
set -u
cd "$(dirname "$0")/.." || exit 2
errors=0
for file in "$@"; do
	while IFS= read -r hit; do
		echo "$file:${hit%%:*}: uses ${hit#*:}"
		errors=$((errors + 1))
	done < <(grep -nE '\bGame\(\)|#include ".*(game_state\.h|/ui/|/input/)' "$file")
	while IFS= read -r hit; do
		echo "$file:${hit%%:*}: draws outside src/graphics/dungeon_render*.cpp: ${hit#*:}"
		errors=$((errors + 1))
	done < <(grep -nE '#include <GL|#include ".*(graphics|render)/' "$file")
done
if [ $errors -gt 0 ]; then
	echo "check_sim: $errors violation(s)"
	exit 1
fi
echo "check_sim: OK"
