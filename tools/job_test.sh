#!/usr/bin/env bash
# Tests for tools/job.sh (plan wait-job-tool); `make check` runs them, a few seconds.
set -u
job=$(realpath "$(dirname "$0")/job.sh")
JOB_DIR=$(mktemp -d)
export JOB_DIR
planted=()
cleanup() {
	local p
	for p in "${planted[@]}"; do kill "$p" 2>/dev/null; done
	for p in "$JOB_DIR"/*/pid; do [ -f "$p" ] && kill -KILL -- "-$(cat "$p")" 2>/dev/null; done
	rm -rf "$JOB_DIR"
}
trap cleanup EXIT
fails=0

fail() {
	echo "FAIL: $*"
	fails=$((fails + 1))
}

# $1: the wanted exit code, then the job.sh arguments. Output in $out.
run() {
	local want=$1 code
	shift
	out=$("$job" "$@" 2>&1)
	code=$?
	[ "$code" = "$want" ] || fail "job.sh $* exited $code, want $want: $out"
}

# Success, failure and a SIGTERM all write the exit file.
run 0 start ok -- bash -c 'echo hello'
run 0 wait ok 5
[[ $out == *hello* ]] || fail "wait ok: no log tail: $out"
run 0 start bad -- bash -c 'echo oops; exit 3'
run 3 wait bad 5
run 0 start term -- sleep 30
pid=$(cat "$JOB_DIR/term/pid")
kill -TERM "$pid"
run 143 wait term 5
kill -0 "$pid" 2>/dev/null && fail "term: wrapper still alive"
pgrep -f "^sleep 30$" -g "$pid" >/dev/null && fail "term: sleep 30 left behind"

# A deadline: 124 in time, the job runs on, no waiter left behind.
run 0 start slow -- sleep 30
t0=$SECONDS
run 124 wait slow 1
[ $((SECONDS - t0)) -le 3 ] || fail "wait slow 1 took $((SECONDS - t0)) s"
[[ $out == running:* ]] || fail "wait slow: $out"
pgrep -f "job.sh wait slow" >/dev/null && fail "a waiter was left behind"

# start refuses a running name; a finished one moves to NAME.prev.
run 2 start slow -- true
run 0 start ok -- true
[ -f "$JOB_DIR/ok.prev/exit" ] || fail "ok.prev missing"
run 0 stop slow
[ "$(cat "$JOB_DIR/slow/exit")" = 143 ] || fail "stop: exit $(cat "$JOB_DIR/slow/exit")"

# kill -9 leaves no exit file: died.
run 0 start hard -- sleep 30
kill -KILL -- "-$(cat "$JOB_DIR/hard/pid")"
sleep 0.2
run 125 wait hard 5
[[ $out == died:* ]] || fail "wait hard: $out"

run 0 status
[[ $out == *"exit 3"* && $out == *died* ]] || fail "status: $out"
run 0 log bad 5
[[ $out == oops ]] || fail "log: $out"

# reap finds a planted wait loop in an agent shell and leaves other shells alone (only this test's processes).
mark=job_test_$$
bash -c ": shell-snapshots $mark; until false; do sleep 1; done" &
loop=$!
bash -c ": shell-snapshots $mark; sleep 30" &
other=$!
planted+=("$loop" "$other")
sleep 0.3
out=$(REAP_AGE=0 REAP_ONLY=$mark "$job" reap)
[[ $out == *"pid $loop,"* ]] || fail "reap missed the loop: $out"
[[ $out == *"pid $other,"* ]] && fail "reap listed a plain shell: $out"
REAP_AGE=0 REAP_ONLY=$mark "$job" reap --kill >/dev/null
sleep 0.3
kill -0 "$loop" 2>/dev/null && fail "reap --kill left the loop"
kill -0 "$other" 2>/dev/null || fail "reap --kill killed a plain shell"

if [ $fails -gt 0 ]; then
	echo "job_test: $fails failed"
	exit 1
fi
echo "job_test: ok"
