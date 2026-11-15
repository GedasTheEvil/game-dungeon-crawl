#!/usr/bin/env bash
# Jobs for long commands: start one detached, wait for it with a deadline, never a hand-written wait loop (plan
# wait-job-tool, docs/stuck-processes.md).
#
#   tools/job.sh start NAME -- CMD...     run CMD detached in the current directory
#   tools/job.sh wait NAME [SECONDS] [--tail N]
#                                         wait for it, at most SECONDS (default 100, at most 590); exits with the job's
#                                         code, 124 when still running, 125 when it died without an exit code
#   tools/job.sh status [NAME]            every job: state, age, log lines, directory
#   tools/job.sh log NAME [N]             the last N lines of its log (default 40)
#   tools/job.sh stop NAME                kill the job (its whole process group); it writes its exit code
#   tools/job.sh reap [--kill]            stray wait loops, processes in removed worktrees, jobs in removed folders
#   tools/job.sh clean                    drop finished jobs older than a day
#
# A job lives in tmp/jobs/NAME/ of the main folder (worktrees too, found with git rev-parse --git-common-dir): log,
# pid, cwd, cmd, start and exit (written on success, failure and a kill). JOB_DIR overrides the folder (tests);
# REAP_AGE (seconds, default 900) the age of a loop reap lists, REAP_ONLY a text the processes reap looks at must hold.
set -u

die() {
	echo "job.sh: $*" >&2
	exit 2
}

jobs_dir() {
	if [ -n "${JOB_DIR:-}" ]; then
		echo "$JOB_DIR"
		return
	fi
	local common
	common=$(git rev-parse --git-common-dir 2>/dev/null) || die "not in a git folder (or set JOB_DIR)"
	echo "$(dirname "$(realpath "$common")")/tmp/jobs"
}

JOBS=$(jobs_dir)

# "1m05s" from seconds.
human() {
	local s=$1
	if [ "$s" -ge 3600 ]; then
		printf '%dh%02dm' $((s / 3600)) $((s % 3600 / 60))
	elif [ "$s" -ge 60 ]; then
		printf '%dm%02ds' $((s / 60)) $((s % 60))
	else
		printf '%ds' "$s"
	fi
}

job_dir() {
	[[ $1 =~ ^[A-Za-z0-9._-]+$ ]] || die "bad job name '$1' (letters, digits, . _ -)"
	echo "$JOBS/$1"
}

# running, done, died or none.
state() {
	local d=$1
	if [ -f "$d/exit" ]; then
		echo 'done'
	elif [ ! -f "$d/pid" ]; then
		echo none
	elif kill -0 "$(cat "$d/pid")" 2>/dev/null; then
		echo running
	elif [ -f "$d/exit" ]; then # it ended between the two tests
		echo 'done'
	else
		echo died
	fi
}

age_of() {
	local d=$1 t0 t1
	t0=$(cat "$d/start" 2>/dev/null || stat -c %Y "$d")
	if [ -f "$d/exit" ]; then t1=$(stat -c %Y "$d/exit"); else t1=$(date +%s); fi
	human $((t1 - t0))
}

log_lines() {
	if [ -f "$1/log" ]; then wc -l <"$1/log"; else echo 0; fi
}

last_line() {
	[ -f "$1/log" ] && tail -n 1 "$1/log" | cut -c1-160
}

# The job's wrapper: runs CMD in its own process group and always writes its exit code. $1: the job folder.
# shellcheck disable=SC2016 # expanded by the wrapper's bash
WRAPPER='
d=$1
shift
echo $$ >"$d/pid"
child=
finish() {
	echo "$1" >"$d/exit.tmp" && mv "$d/exit.tmp" "$d/exit"
}
on_signal() {
	trap "" TERM INT HUP
	kill -TERM -- -$$ 2>/dev/null
	finish "$1"
	exit "$1"
}
trap "on_signal 143" TERM
trap "on_signal 130" INT
trap "on_signal 129" HUP
"$@" &
child=$!
wait "$child"
code=$?
finish "$code"
exit "$code"
'

cmd_start() {
	[ $# -ge 1 ] || die "start NAME -- CMD..."
	local name=$1 d
	d=$(job_dir "$name")
	shift
	[ "${1:-}" = "--" ] && shift
	[ $# -ge 1 ] || die "start $name: no command"
	if [ -d "$d" ]; then
		[ "$(state "$d")" = running ] && die "job $name is still running (pid $(cat "$d/pid")): wait for it or stop it"
		rm -rf "$d.prev"
		mv "$d" "$d.prev"
	fi
	mkdir -p "$d"
	pwd >"$d/cwd"
	printf '%q ' "$@" >"$d/cmd"
	echo >>"$d/cmd"
	date +%s >"$d/start"
	setsid bash -c "$WRAPPER" job-wrapper "$d" "$@" >"$d/log" 2>&1 </dev/null &
	for _ in $(seq 50); do
		[ -s "$d/pid" ] && break
		sleep 0.1
	done
	[ -s "$d/pid" ] || die "job $name did not start"
	echo "started $name: pid $(cat "$d/pid"), log $d/log"
}

cmd_wait() {
	local name='' secs=100 tail_n=15
	while [ $# -gt 0 ]; do
		case $1 in
		--tail)
			tail_n=$2
			shift 2
			;;
		*)
			if [ -z "$name" ]; then name=$1; else secs=$1; fi
			shift
			;;
		esac
	done
	[ -n "$name" ] || die "wait NAME [SECONDS] [--tail N]"
	[[ $secs =~ ^[0-9]+$ ]] || die "SECONDS must be a number"
	[ "$secs" -gt 590 ] && secs=590
	local d s
	d=$(job_dir "$name")
	[ -d "$d" ] || die "no job $name (tools/job.sh status)"
	local deadline=$((SECONDS + secs))
	while :; do
		s=$(state "$d")
		case $s in
		done)
			local code
			code=$(cat "$d/exit")
			echo "done: $name, exit $code, $(age_of "$d"), log $d/log"
			tail -n "$tail_n" "$d/log"
			exit "$code"
			;;
		died | none)
			echo "died: $name, no exit code (killed hard?), $(age_of "$d"), log $d/log"
			tail -n "$tail_n" "$d/log"
			exit 125
			;;
		esac
		if [ "$SECONDS" -ge "$deadline" ]; then
			echo "running: $name, $(age_of "$d"), log $(log_lines "$d") lines, last: $(last_line "$d")"
			exit 124
		fi
		sleep 0.5
	done
}

cmd_status() {
	local d name s
	if [ $# -ge 1 ]; then
		set -- "$(job_dir "$1")"
		[ -d "$1" ] || die "no job ${1##*/}"
	else
		set -- "$JOBS"/*/
	fi
	for d in "$@"; do
		d=${d%/}
		[ -d "$d" ] || continue
		name=${d##*/}
		s=$(state "$d")
		[ "$s" = "done" ] && s="exit $(cat "$d/exit")"
		printf '%-24s %-9s %8s %6s lines  %s\n' "$name" "$s" "$(age_of "$d")" "$(log_lines "$d")" "$(cat "$d/cwd" 2>/dev/null)"
	done
}

cmd_log() {
	[ $# -ge 1 ] || die "log NAME [N]"
	local d
	d=$(job_dir "$1")
	[ -f "$d/log" ] || die "no job $1"
	tail -n "${2:-40}" "$d/log"
}

cmd_stop() {
	[ $# -ge 1 ] || die "stop NAME"
	local d
	d=$(job_dir "$1")
	[ "$(state "$d")" = running ] || die "job $1 is not running"
	kill -TERM "$(cat "$d/pid")"
	for _ in $(seq 50); do
		[ -f "$d/exit" ] && break
		sleep 0.1
	done
	echo "stopped $1: exit $(cat "$d/exit" 2>/dev/null || echo '?')"
}

# The pids of this shell and its parents: reap never lists them.
ancestors() {
	local p=$$
	while [ "$p" -gt 1 ]; do
		echo "$p"
		p=$(ps -o ppid= -p "$p" | tr -d ' ')
		[ -n "$p" ] || break
	done
}

kill_tree() {
	local p=$1 c
	for c in $(ps -o pid= --ppid "$p"); do kill_tree "$c"; done
	kill -TERM "$p" 2>/dev/null
}

cmd_reap() {
	local kill=0 min_age=${REAP_AGE:-900} found=0
	[ "${1:-}" = --kill ] && kill=1
	local skip pid etimes args cwd
	skip=" $(ancestors | tr '\n' ' ') "
	while read -r pid etimes args; do
		[[ $skip == *" $pid "* ]] && continue
		[[ -n ${REAP_ONLY:-} && $args != *"$REAP_ONLY"* ]] && continue # tests: only their own processes
		# Wait loops in the agents' shells (their command line sources a shell snapshot).
		if [[ $args == *shell-snapshots* ]] && [[ $args =~ (until|while)[[:space:]].*sleep ]] && [ "$etimes" -ge "$min_age" ]; then
			echo "loop     pid $pid, $(human "$etimes"): ${args:0:200}"
			found=1
			[ $kill = 1 ] && kill_tree "$pid"
			continue
		fi
		cwd=$(readlink "/proc/$pid/cwd" 2>/dev/null) || continue
		if [[ $cwd == *" (deleted)" ]]; then
			echo "deleted  pid $pid, $(human "$etimes"), in $cwd: ${args:0:160}"
			found=1
			[ $kill = 1 ] && kill_tree "$pid"
		fi
	done < <(ps -u "$(id -u)" -o pid=,etimes=,args=)
	local d
	for d in "$JOBS"/*/; do
		d=${d%/}
		[ -d "$d" ] && [ "$(state "$d")" = running ] || continue
		cwd=$(cat "$d/cwd" 2>/dev/null)
		[ -d "$cwd" ] && continue
		echo "job      ${d##*/}, pid $(cat "$d/pid"), $(age_of "$d"), folder gone: $cwd"
		found=1
		[ $kill = 1 ] && kill -TERM "$(cat "$d/pid")" 2>/dev/null
	done
	if [ $found = 0 ]; then
		echo "nothing to reap"
	elif [ $kill = 0 ]; then
		echo "(tools/job.sh reap --kill kills them)"
	fi
}

cmd_clean() {
	local d s n=0
	for d in "$JOBS"/*/; do
		d=${d%/}
		[ -d "$d" ] || continue
		s=$(state "$d")
		[ "$s" = running ] && continue
		# The folder's time is its last new file: the exit file, or the start of a job that died.
		[ -n "$(find "$d" -maxdepth 0 -mmin +1440)" ] || continue
		rm -rf "$d"
		n=$((n + 1))
	done
	echo "removed $n finished jobs older than a day"
}

cmd=${1:-}
[ $# -ge 1 ] && shift
case $cmd in
start) cmd_start "$@" ;;
wait) cmd_wait "$@" ;;
status) cmd_status "$@" ;;
log) cmd_log "$@" ;;
stop) cmd_stop "$@" ;;
reap) cmd_reap "$@" ;;
clean) cmd_clean "$@" ;;
*) sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//' && exit 2 ;;
esac
