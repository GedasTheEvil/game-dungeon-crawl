#!/usr/bin/env bash
# Checks that the world and the entities stay off the app (docs/plan/solved/world-without-game.md): no Game(), no
# game_state.h, no screens (ui/) or input. What they need comes in by SimLinks / MonsterLinks or a parameter, and what
# they tell the app goes out by WorldEvents.
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
done
if [ $errors -gt 0 ]; then
	echo "check_sim: $errors violation(s)"
	exit 1
fi
echo "check_sim: OK"
