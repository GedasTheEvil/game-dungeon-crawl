#!/bin/bash
# The release zip on other distros (plan portable-linux-release): per distro a container with only Xvfb, Mesa and unzip
# added (images cached as dungeon-crawl-smoke:<name>), the zip unzipped, Install-check, then a short scenario (level 1
# loads, a frame renders) on Xvfb without sound. Pass: Install-check exit 0, the game exit 0 and its
# screenshot. Pulls images: not part of make check; tools/release.sh --smoke (make release ARGS=--smoke) runs it.
# Ubuntu 20.04 has glibc 2.31, the release's floor. Debian 11 (also 2.31) is gone: its security archive no longer serves.
# Usage: tools/release_smoke.sh ZIP [NAME...]   (names: mint22 ubuntu2204 ubuntu2004 fedora; default all)
set -eu
cd "$(dirname "$0")/.."

[ $# -ge 1 ] && [ -f "$1" ] || { echo "usage: tools/release_smoke.sh ZIP [mint22|ubuntu2204|ubuntu2004|fedora...]" >&2; exit 2; }
zip=$(realpath "$1")
shift
names=("$@")
[ ${#names[@]} = 0 ] && names=(mint22 ubuntu2204 ubuntu2004 fedora)

APT_PACKAGES="xvfb libgl1 libgl1-mesa-dri unzip"
base_of() {
	case "$1" in
		mint22) echo linuxmintd/mint22-amd64 ;;
		ubuntu2204) echo ubuntu:22.04 ;;
		ubuntu2004) echo ubuntu:20.04 ;;
		fedora) echo fedora:latest ;;
		*) echo "release_smoke: unknown distro $1" >&2; exit 2 ;;
	esac
}
install_of() {
	case "$1" in
		fedora) echo "dnf -y -q install xorg-x11-server-Xvfb mesa-dri-drivers mesa-libGL mesa-libEGL unzip && dnf clean all" ;;
		*) echo "apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends $APT_PACKAGES && rm -rf /var/lib/apt/lists/*" ;;
	esac
}

# Level 1 from the zip's content/, a second of play, a screenshot; the game's exit code says whether the expects held.
scenario='seed 1
level 1
wait 60
screenshot smoke
expect alive == 1
expect level == 1
quit'

# In the container: the zip in /zip.zip, the game unzipped as the player would.
run='set -e
mkdir /tmp/game && cd /tmp/game && unzip -q /zip.zip
echo "-- Install-check"; ./Install-check
printf "%s\n" "$SCENARIO" > smoke.txt
Xvfb :99 -screen 0 1280x720x24 -nolisten tcp >/dev/null 2>&1 &
sleep 2
echo "-- game"
DISPLAY=:99 timeout -k 5 60 bin/game --no-sound smoke.txt || { rc=$?; tail -20 game.log; exit $rc; }
test -n "$(ls tests/out/smoke/*.png 2>/dev/null)" || { echo "no screenshot"; tail -20 game.log; exit 1; }
grep -m1 "OpenGL" game.log || true'

failed=()
for name in "${names[@]}"; do
	image=dungeon-crawl-smoke:$name
	echo "== $name ($(base_of "$name"))"
	if ! docker image inspect "$image" >/dev/null 2>&1; then
		if ! printf 'FROM %s\nRUN %s\n' "$(base_of "$name")" "$(install_of "$name")" | docker build -q -t "$image" - >/dev/null; then
			echo "== $name: FAIL (image build)"
			failed+=("$name")
			continue
		fi
	fi
	if docker run --rm -v "$zip:/zip.zip:ro" -e SCENARIO="$scenario" "$image" bash -c "$run"; then
		echo "== $name: ok"
	else
		echo "== $name: FAIL"
		failed+=("$name")
	fi
done
if [ ${#failed[@]} != 0 ]; then
	echo "release_smoke: failed on ${failed[*]}" >&2
	exit 1
fi
echo "release_smoke: ok (${names[*]})"
