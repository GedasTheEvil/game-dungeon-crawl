"""Fetch the zone and menu music (plan zone-music) from its free sources and write it to the game's level.

    python3 tools/audio/zone_music.py [slug ...]      (default: every track)

TRACKS names, per music file (media/sounds/music/<slug>.ogg), its source page, download, author and licence. Only
CC0 tracks, so no credit line is needed in the game; media/sounds/music/CREDITS.md lists them all anyway.
The downloads are cached in build/music-src/. Every track is loudness-normalised to the same level (TARGET_LUFS,
EBU R128, two-pass loudnorm) and encoded to Ogg Vorbis, 44.1 kHz stereo. To swap a track: change its row, rerun.
Needs ffmpeg.
"""

import json
import os
import subprocess
import sys
import urllib.parse
import urllib.request
import zipfile

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(REPO, "media", "sounds", "music")
CACHE = os.path.join(REPO, "build", "music-src")
TARGET_LUFS = -16  # a little under soundtrack.ogg (-13), the zone ambience plays on top
QUALITY = "4"  # libvorbis -q, ~128 kb/s
OGA = "https://opengameart.org/"

# slug: (title, author, licence, page, download, file inside the zip or None)
TRACKS = {
    "menu": ("Desert_mystic 2 (dynastic Egyptian theme)", "PolygonDan", "CC0",
             OGA + "content/desertmystic-dynastic-egyptian-theme-cc0",
             OGA + "sites/default/files/Desert_Mystic%20By%20Polygondan.zip", "Desert Mystic2.mp3"),
    "tomb_entrance": ("Forgoten tomb ambience", "kindland", "CC0", OGA + "content/forgoten-tomb-ambience",
                      OGA + "sites/default/files/Forgoten_tombs_1.mp3", None),
    "bat_galleries": ("Cave 2", "HoliznaCC0", "CC0", OGA + "content/caves-collection",
                      OGA + "sites/default/files/caves_zip_0.zip", "Caves Zip/01 HoliznaCC0 - Cave 2.ogg"),
    "web_halls": ("Dark Cavern Ambient (loop)", "Paul Wortmann", "CC0", OGA + "content/dark-cavern-ambient",
                  OGA + "sites/default/files/dark_cavern_ambient_002.ogg", None),
    "priests_quarter": ("Egyptian meditation music", "brandon75689", "CC0 (or OGA-BY 3.0)",
                        OGA + "content/egyptian-meditation-music", OGA + "sites/default/files/v312.ogg", None),
    "scorpion_nest": ("Desert theme", "yd", "CC0", OGA + "content/desert-theme",
                      OGA + "sites/default/files/caravan.ogg.ogg", None),
    "serpent_temple": ("Ancient Power Of Serpents", "Kevin MacLeod (FreePD), uploaded by josepharaoh99", "CC0",
                       OGA + "content/ancient-power-of-serpents",
                       OGA + "sites/default/files/Ancient_Power_Of_Serpents_0.ogg", None),
    "cisterns_of_sobek": ("The Seeing Pool (no sfx)", "Umplix", "CC0", OGA + "content/the-seeing-pool",
                          OGA + "sites/default/files/the_seeing_pool_no_sfx.wav", None),
    "seths_desert": ("Un désert", "Komiku (Loyalty Freak Music)", "CC0", OGA + "content/un-d%C3%A9sert",
                     OGA + "sites/default/files/Komiku_-_08_-_Un_dsert_0.mp3", None),
    "land_of_sokar": ("Osiris Monolith (2024)", "Bobjt", "CC0", OGA + "content/osiris-monolith",
                      OGA + "sites/default/files/2-osiris_monolith-2024.ogg", None),
    "lake_of_fire": ("Evil temple Ambiance loop", "HaelDB", "CC0 (or OGA-BY 3.0)",
                     OGA + "content/evil-temple-ambiance-loop", OGA + "sites/default/files/evil_temple.ogg", None),
    "necropolis": ("Dungeon Ambience", "yd", "CC0", OGA + "content/dungeon-ambience",
                   OGA + "sites/default/files/dungeon002_0.ogg", None),
    "hall_of_judgment": ("Great Labyrinth (orchestral dungeon music)", "MintoDog", "CC0",
                         OGA + "content/great-labyrinthorchestral-dungeon-music",
                         OGA + "sites/default/files/great_labyrinth_bpm160_0.ogg", None),
}


def download(url):
    path = os.path.join(CACHE, urllib.parse.unquote(url.rsplit("/", 1)[1]))
    if not os.path.exists(path):
        print(f"  download {url}")
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=120) as r, open(path + ".part", "wb") as f:
            f.write(r.read())
        os.replace(path + ".part", path)
    return path


def source(url, member):
    path = download(url)
    if member is None:
        return path
    unpacked = path + ".d"
    if not os.path.exists(os.path.join(unpacked, member)):
        with zipfile.ZipFile(path) as z:
            z.extract(member, unpacked)
    return os.path.join(unpacked, member)


def ffmpeg(*args):
    return subprocess.run(["ffmpeg", "-hide_banner", "-nostdin", *args], capture_output=True, text=True, check=True)


def encode(src, dst):
    loudnorm = f"loudnorm=I={TARGET_LUFS}:TP=-1.5:LRA=11"
    stats = ffmpeg("-i", src, "-af", loudnorm + ":print_format=json", "-f", "null", "-").stderr
    m = json.loads(stats[stats.rindex("{"):stats.rindex("}") + 1])
    measured = (f":measured_I={m['input_i']}:measured_TP={m['input_tp']}:measured_LRA={m['input_lra']}"
                f":measured_thresh={m['input_thresh']}:offset={m['target_offset']}:linear=true")
    ffmpeg("-y", "-i", src, "-vn", "-map_metadata", "-1", "-af", loudnorm + measured, "-ar", "44100", "-ac", "2",
           "-c:a", "libvorbis", "-q:a", QUALITY, dst)
    return float(m["input_i"])


def credits():
    lines = ["# Music credits", "",
             "The zone and menu music (plan zone-music). Every track is CC0 (public domain): no credit is needed,",
             "these are given anyway. Written by tools/audio/zone_music.py from its table; edit the table, not this.",
             "`soundtrack.ogg` is not listed: it is not free and not in git.", "",
             "| File | Track | Author | Licence | Source |", "|---|---|---|---|---|"]
    for slug, (title, author, licence, page, _, _) in TRACKS.items():
        lines.append(f"| `{slug}.ogg` | {title} | {author} | {licence} | <{page}> |")
    with open(os.path.join(OUT, "CREDITS.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main():
    slugs = sys.argv[1:] or list(TRACKS)
    os.makedirs(CACHE, exist_ok=True)
    for slug in slugs:
        title, _, _, _, url, member = TRACKS[slug]
        print(f"{slug}: {title}")
        was = encode(source(url, member), os.path.join(OUT, slug + ".ogg"))
        print(f"  {was:.1f} LUFS -> {TARGET_LUFS}")
    credits()


if __name__ == "__main__":
    main()
