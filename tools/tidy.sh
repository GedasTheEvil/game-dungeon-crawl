#!/usr/bin/env bash
# Runs clang-tidy, one process per source through the load gate, and fails on any warning (plan dev-friction-review).
#
#   tools/tidy.sh changed|all SOURCES... -- TIDY_FLAGS...
#
# all: every source. changed: only what differs from the merge base with master (TIDY_BASE: another base; committed on a
# task branch, staged, unstaged or new): the changed sources, and for each changed header no changed source includes
# (also through other headers, the build's .d files) one source that includes it: a header's warnings show in any
# source that includes it (plan tidy-header-fanout). TIDY_FANOUT=all tidies every including source (`make tidy-push`).
# The tree is clean, so every warning printed is the run's own. Drops clang's "N warnings generated." lines (they count the warnings
# filtered out, e.g. in system headers).
#
# A source that came out clean is cached (plan check-speed-stage-breakdown): the key is a hash of the source, every
# repo header it includes (the build's .d file), .clang-tidy, the flags and the clang-tidy version, so a cache hit
# means the same input: a re-run after a header change (the fix loop) tidies only what the fix touched. The cache is in
# the git common dir (shared by the worktrees; entries unused for 30 days go). TIDY_CACHE=0 turns it off. A source
# without a .d file (not built yet) is never cached.
set -u
cd "$(dirname "$0")/.." || exit 2
mode=$1
cache=$(realpath "$(git rev-parse --git-common-dir)")/tidy-cache

# tidy.sh one SOURCE -- FLAGS...: one source through the cache and the gate; prints its warnings.
if [ "$mode" = one ]; then
	src=$2
	shift 3
	key=''
	dep=build/${src%.cpp}.d
	if [ "${TIDY_CACHE:-1}" != 0 ] && [ -f "$dep" ]; then
		# shellcheck disable=SC2046
		key=$({
			"${CLANG_TIDY:-clang-tidy}" --version
			echo "$*"
			cat .clang-tidy "$src"
			cat $(sed 's/[\\:]/ /g' "$dep" | tr ' ' '\n' | grep -E '^(src|tools|external)/|/(src|tools|external)/' | sort -u | sed "s|^$PWD/||" | while read -r f; do [ -f "$f" ] && echo "$f"; done)
		} | sha256sum | cut -c1-40)
		[ -e "$cache/$key" ] && exit 0
	fi
	out=$(./tools/load_gate.sh run "${CLANG_TIDY:-clang-tidy}" --quiet "$src" -- "$@" 2>&1 | sed '/^[0-9]* warnings\? generated\.$/d')
	[ -n "$out" ] && echo "$out"
	if [ -n "$key" ] && ! grep -qE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' <<<"$out"; then
		mkdir -p "$cache" && : >"$cache/$key"
	fi
	exit 0
fi
shift
sources=()
while [ $# -gt 0 ] && [ "$1" != "--" ]; do
	sources+=("$1")
	shift
done
shift

# The repo headers SOURCE includes, also through other headers: the build's dependency file, else its own includes.
deps_of() {
	local src=$1 dep="build/${1%.cpp}.d"
	if [ -f "$dep" ]; then
		# shellcheck disable=SC2046
		realpath -m --relative-to=. $(sed 's/[\\:]/ /g' "$dep") | sort -u
	else
		grep -oE '#include "[^"]+"' "$src" | cut -d'"' -f2 | sed "s|^|$(dirname "$src")/|" |
			xargs -r realpath -m --relative-to=.
	fi
}

if [ "$mode" = changed ]; then
	base=$(git merge-base HEAD "${TIDY_BASE:-master}" 2>/dev/null || echo HEAD)
	changed=$({ git diff --name-only "$base"; git ls-files --others --exclude-standard; } | sort -u)
	headers=$(grep -E '\.h$' <<<"$changed")
	picked=()
	declare -A covered=()   # changed header -> 1 once a picked source includes it
	declare -A includers=() # changed header -> the unpicked sources that include it, one per line
	declare -A weight=()    # source -> the number of headers it includes (a stand-in's cost)
	for src in "${sources[@]}"; do
		is_changed=0
		grep -qxF "$src" <<<"$changed" && is_changed=1
		[ -n "$headers" ] || {
			[ $is_changed = 1 ] && picked+=("$src")
			continue
		}
		deps=$(deps_of "$src")
		hit=$(grep -xFf <(echo "$headers") <<<"$deps")
		if [ $is_changed = 1 ] || { [ -n "$hit" ] && [ "${TIDY_FANOUT:-one}" = all ]; }; then
			picked+=("$src")
			for h in $hit; do covered[$h]=1; done
			continue
		fi
		[ -n "$hit" ] || continue
		weight[$src]=$(grep -c . <<<"$deps")
		for h in $hit; do includers[$h]+="$src"$'\n'; done
	done
	# A changed header no picked source includes: one source stands in for it (plan tidy-header-fanout), its own .cpp
	# when there is one, else the one with the fewest includes. A header's warnings show in any source that includes
	# it; a warning in a caller's own code waits for the full fan-out (TIDY_FANOUT=all, `make tidy-push`).
	for h in $headers; do
		[ -z "${covered[$h]:-}" ] && [ -n "${includers[$h]:-}" ] || continue
		own=${h%.h}.cpp
		if grep -qxF "$own" <<<"${includers[$h]}"; then
			pick=$own
		else
			pick=$(while read -r s; do [ -n "$s" ] && echo "${weight[$s]} $s"; done <<<"${includers[$h]}" | sort -n |
				head -n 1 | cut -d' ' -f2)
		fi
		n=$(grep -c . <<<"${includers[$h]}")
		echo "tidy: $pick stands in for $h ($n including sources; TIDY_FANOUT=all tidies them all)"
		picked+=("$pick")
		while read -r c; do covered[$c]=1; done < <(grep -xFf <(echo "$headers") <<<"$(deps_of "$pick")")
	done
	sources=("${picked[@]+"${picked[@]}"}")
	echo "tidy: ${#sources[@]} source(s) for the changes since ${base:0:7} (make tidy-all checks every source)"
fi
[ -n "${CHECK_STAGE_NOTE:-}" ] && echo "${#sources[@]}" >"$CHECK_STAGE_NOTE"
[ ${#sources[@]} -eq 0 ] && exit 0

[ -d "$cache" ] && find "$cache" -type f -mtime +30 -delete
out=$(printf '%s\n' "${sources[@]}" | xargs -P $(($(./tools/load_gate.sh jobs) + 1)) -I{} \
	./tools/tidy.sh one {} -- "$@" 2>&1)
[ -n "$out" ] && echo "$out"
# Any warning or error fails the run; a header's warning repeats once per including source, count it once.
count=$(grep -oE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' <<<"$out" | sort -u | wc -l)
if [ "$count" -gt 0 ]; then
	echo "tidy: $count warning(s)"
	exit 1
fi
exit 0
