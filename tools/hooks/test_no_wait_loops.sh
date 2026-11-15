#!/usr/bin/env bash
# shellcheck disable=SC2016 # the commands are text, never run
# Tests for tools/hooks/no_wait_loops.sh (plan wait-job-tool); `make check` runs them.
set -u
hook=$(dirname "$0")/no_wait_loops.sh
fails=0

# $1: block or pass, $2: the command.
expect() {
	local got=pass
	jq -n --arg c "$2" '{tool_name: "Bash", tool_input: {command: $c}}' | bash "$hook" 2>/dev/null || got=block
	if [ "$got" != "$1" ]; then
		echo "FAIL: want $1, got $got: $2"
		fails=$((fails + 1))
	fi
}

expect block 'until grep -q "^done" /tmp/x.log; do sleep 5; done'
expect block 'until ! pgrep -f "make check"; do sleep 3; done; echo ok'
expect block 'while [ ! -s out.txt ]; do sleep 2; done'
expect block 'while kill -0 123 2>/dev/null; do echo .; sleep 10; done'
expect block $'until [ -f done ]\ndo\n  sleep 1\ndone'
expect block 'timeout 600 bash -c "until grep -q passed log; do sleep 5; done"'
expect pass 'for f in *; do echo "$f"; done'
expect pass 'sleep 2'
expect pass 'make check'
expect pass 'while read -r l; do echo "$l"; done < file'
expect pass 'tools/job.sh wait check 590'
expect pass 'grep -n "until" src/a.cpp; echo sleeper'
expect pass $'cat > doc.md <<\'EOF\'\nNever `until grep -q x log; do sleep 3; done`.\nEOF'
expect block $'until [ -s out ]; do sleep 1; done; cat <<EOF\nx\nEOF'

if [ $fails -gt 0 ]; then
	echo "no_wait_loops: $fails failed"
	exit 1
fi
echo "no_wait_loops: ok"
