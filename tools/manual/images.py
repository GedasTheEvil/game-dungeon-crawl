#!/usr/bin/env python3
"""Make the player manual's pictures (plan player-manual) in docs/manual/images/.

    tools/manual/images.py

* Monsters: one frame of each model in bin/model-viewer (headless), the viewer's flat backdrop keyed out to
  transparency, cropped to the model.
* Screens: tools/manual/shots.txt run through bin/game (the HUD, the inventory, the Heka screen), cropped.
* Textures: the menus' backdrop and the papyrus.
* Icons: cells of the skill and HUD icon atlases (media/textures/ui/), conditions and curses on their status cards.

Pictures no page uses are removed at the end. Needs `make` and `make model-viewer` first.
"""

import os
import subprocess
import sys
import tempfile

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "docs/manual/images")
UI = os.path.join(ROOT, "media/textures/ui")
BACKDROP = (38, 38, 51)  # the model viewer's glClearColor 0.15, 0.15, 0.2
KEY_TOLERANCE = 10
MONSTER_SIDE = 256  # a monster picture's longer side
ICON_SIDE = 64

# (file name, model, texture, yaw, pitch)
MONSTERS = [
    ("rat", "rat", "rat", 60, 15),
    ("giant_rat", "rat", "rat_giant", 60, 15),
    ("scarab", "scarab", "scarab", 60, 25),
    ("giant_scarab", "scarab", "scarab_giant", 60, 25),
    ("cobra", "cobra", "cobra", 60, 10),
    ("giant_cobra", "cobra", "cobra_giant", 60, 10),
    ("khepri", "scarab", "scarab_boss", 60, 25),
    ("mimic", "mimic", "mimic", 60, 15),
]

# The screenshots of shots.txt (1280 x 800) and the parts kept: (shot, file name, box or None for all, width).
SHOTS = [
    ("001_hud.png", "hud", (0, 630, 520, 800), 520),
    ("002_inventory_potions.png", "inventory_potions", None, 800),
    ("003_heka.png", "heka", None, 800),
]

# skill_icons.png: 4 columns of 128 px, the heka mark, then the skills in SkillId order (src/level/skills.h).
SKILL_ICONS = [
    "heka", "stun", "words_of_thoth", "dodge", "power_hit", "eye_of_horus", "favour_of_hapi", "serkets_favour",
    "blessing_of_isis", "flaming_arrows", "ka_of_osiris", "strength_of_montu", "djed_of_ptah", "breath_of_shu",
    "speed_of_mafdet", "wisdom_of_seshat", "disarm_trap", "quiver_of_neith", "bleed",
    "arrows_of_satet", "sting_of_serket", "fang_of_wadjet", "touch_of_imhotep",
]  # fmt: skip

# hud_icons.png: 8 columns of 128 px (PlayerHud::Icon, src/level/hud_icon.h). status_cards.png: blessing, condition,
# curse, 128 px each. (file name, icon cell, card, tint: the status list's, src/level/status_list.cpp)
CARD_BLESSING, CARD_CONDITION, CARD_CURSE = 0, 1, 2
FIRST_CURSE = 37
CURSES = ["weakness", "clumsiness", "vulnerability", "fatigue", "disease", "slowness", "palsy", "insanity"]
STATUS = [
    ("poison", 5, CARD_CONDITION, (0.35, 0.78, 0.2)),
    ("bleeding", 5, CARD_CONDITION, (0.75, 0.06, 0.06)),
    ("burning", 6, CARD_CONDITION, None),
    ("daze", 3, CARD_CONDITION, None),
    ("isis", 4, CARD_BLESSING, None),
    ("montu", 7, CARD_BLESSING, None),
    ("ptah", FIRST_CURSE + len(CURSES), CARD_BLESSING, None),
    ("amun", FIRST_CURSE + len(CURSES) + 1, CARD_BLESSING, None),
    ("serket", FIRST_CURSE + len(CURSES) + 2, CARD_BLESSING, None),
    ("hapi", 5, CARD_BLESSING, (0.08, 0.52, 0.68)),
] + [(f"curse_{name}", FIRST_CURSE + i, CARD_CURSE, None) for i, name in enumerate(CURSES)]


def run(command):
    subprocess.run(command, check=True, capture_output=True, cwd=ROOT)


def render_monster(model, texture, yaw, pitch, tmp):
    run(
        ["xvfb-run", "-a", "-s", "-screen 0 1280x1024x24", "timeout", "-k", "5", "60", "bin/model-viewer",
         f"media/models/monsters/{model}.md3", "--shot", tmp, "--frames", "0", "--yaw", str(yaw),
         "--pitch", str(pitch), "--texture", texture, "--size", "512x512"]
    )  # fmt: skip
    return os.path.join(tmp, model + "_00.png")


def cut_out(path):
    image = Image.open(path).convert("RGBA")
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, _ = pixels[x, y]
            if max(abs(r - BACKDROP[0]), abs(g - BACKDROP[1]), abs(b - BACKDROP[2])) <= KEY_TOLERANCE:
                pixels[x, y] = (0, 0, 0, 0)
    box = image.getbbox()
    if box is None:
        sys.exit(f"{path}: nothing rendered")
    image = image.crop(box)
    image.thumbnail((MONSTER_SIDE, MONSTER_SIDE), Image.LANCZOS)
    return image


def save(image, name):
    target = os.path.join(OUT, name + ".png")
    image.save(target, optimize=True)
    print(os.path.relpath(target, ROOT), image.size)


def monsters():
    for name, model, texture, yaw, pitch in MONSTERS:
        with tempfile.TemporaryDirectory() as tmp:
            save(cut_out(render_monster(model, texture, yaw, pitch, tmp)), f"monster_{name}")


def screens():
    run(
        ["xvfb-run", "-a", "-s", "-screen 0 1280x800x24", "timeout", "-k", "5", "180", "bin/game", "--no-sound",
         "tools/manual/shots.txt"]
    )  # fmt: skip
    for shot, name, box, width in SHOTS:
        image = Image.open(os.path.join(ROOT, "tests/out/shots", shot)).convert("RGB")
        if box is not None:
            image = image.crop(box)
        image = image.resize((width, round(image.height * width / image.width)), Image.LANCZOS)
        save(image.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE), f"screen_{name}")


def cell(atlas, index, columns):
    return atlas.crop(((index % columns) * 128, (index // columns) * 128, (index % columns + 1) * 128,
                       (index // columns + 1) * 128))  # fmt: skip


def tinted(icon, tint):
    if tint is None:
        return icon
    r, g, b, a = icon.split()
    scale = [lambda v, f=f: round(v * f) for f in tint]
    return Image.merge("RGBA", (r.point(scale[0]), g.point(scale[1]), b.point(scale[2]), a))


# hud_icons.png's item cells: the weapons from 8 in ItemKind order, the amulet types from 21 (AmuletType order).
WEAPON_ICONS = ["club", "knife", "sword", "khopesh", "epsilon_axe", "duckbill_axe", "mace", "spear", "bow",
                "chariot_bow", "sling", "throwing_stick", "javelin"]  # fmt: skip
AMULET_ICONS = ["strength", "armor", "health", "poison_guard", "trap_guard", "smash_guard", "cut_guard", "stab_guard",
                "regeneration", "venom", "fire_guard", "crushing", "cleaving", "piercing", "shots", "quick_shots"]  # fmt: skip


# The potions on hud_icons.png's white potion cell (0), tinted with their colour (ITEMS, src/level/items.cpp).
POTIONS = [
    ("small_health", (1.0, 0.0, 0.0)), ("large_health", (0.7, 0.0, 0.3)), ("might", (0.4, 0.0, 0.6)),
    ("armor", (1.0, 0.6, 0.0)), ("life", (0.7, 0.6, 0.3)), ("small_stamina", (0.45, 0.85, 0.25)),
    ("large_stamina", (0.15, 0.78, 0.72)), ("antidote", (0.05, 0.45, 0.2)), ("lesser_resistance", (0.35, 0.62, 0.92)),
    ("greater_resistance", (0.12, 0.22, 0.78)), ("water_blessing", (0.08, 0.52, 0.68)),
]  # fmt: skip


# The ring cell's blue gem recoloured per gem (RING_GEMS, src/level/items.cpp).
RING_GEMS = [
    ("garnet", (0.78, 0.04, 0.10)), ("lapis", (0.10, 0.28, 0.95)), ("feldspar", (0.05, 0.75, 0.30)),
    ("jasper", (1.0, 0.92, 0.25)), ("crystal", (0.82, 0.92, 1.0)), ("amethyst", (0.62, 0.22, 0.92)),
]  # fmt: skip


def ring(icon, colour):
    """The ring cell with its gem (the blue pixels) in the colour, its shading kept."""
    image = icon.copy()
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if b > r + 40 and b > g + 20:
                light = b / 255 * 1.4
                pixels[x, y] = tuple(min(255, round(255 * c * light)) for c in colour) + (a,)
    return image


def icons():
    skills = Image.open(os.path.join(UI, "skill_icons.png")).convert("RGBA")
    for i, name in enumerate(SKILL_ICONS):
        save(cell(skills, i, 4).resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"skill_{name}")
    hud = Image.open(os.path.join(UI, "hud_icons.png")).convert("RGBA")
    for i, name in enumerate(WEAPON_ICONS):
        save(cell(hud, 8 + i, 8).resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"weapon_{name}")
    for i, name in enumerate(AMULET_ICONS):
        save(cell(hud, 21 + i, 8).resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"amulet_{name}")
    for name, colour in POTIONS:
        save(tinted(cell(hud, 0, 8), colour).resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"potion_{name}")
    for name, colour in RING_GEMS:
        save(ring(cell(hud, 2, 8), colour).resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"ring_{name}")
    cards = Image.open(os.path.join(UI, "status_cards.png")).convert("RGBA")
    for name, index, card, tint in STATUS:
        image = cell(cards, card, 3)
        icon = tinted(cell(hud, index, 8), tint).resize((100, 100), Image.LANCZOS)
        image.alpha_composite(icon, (14, 14))
        save(image.resize((ICON_SIDE, ICON_SIDE), Image.LANCZOS), f"status_{name}")


def textures():
    """The backdrop (the carved wall, tinted as ui::backdrop does) and the papyrus (its torn edges cropped as the
    game's UVs 0.04, 0.07 - 0.96, 0.93 do)."""
    wall = Image.open(os.path.join(UI, "scarab_slate.png")).convert("RGB")
    tint = (0.34, 0.27, 0.20)
    save(Image.merge("RGB", [band.point(lambda v, f=f: round(v * f)) for band, f in zip(wall.split(), tint)]),
         "backdrop")  # fmt: skip
    papyrus = Image.open(os.path.join(UI, "papyrus_sheet.png")).convert("RGB")
    w, h = papyrus.size
    save(papyrus.crop((round(w * 0.04), round(h * 0.07), round(w * 0.96), round(h * 0.93))), "papyrus")


def prune():
    """Removes the pictures no page of the manual uses (the atlases give more than it shows)."""
    manual = os.path.dirname(OUT)
    used = ""
    for page in os.listdir(manual):
        if page.endswith((".html", ".css")):
            with open(os.path.join(manual, page), encoding="utf-8") as f:
                used += f.read()
    for picture in sorted(os.listdir(OUT)):
        if f"images/{picture}" not in used:
            os.remove(os.path.join(OUT, picture))
            print("unused, removed:", picture)


def main():
    os.makedirs(OUT, exist_ok=True)
    parts = sys.argv[1:] or ["monsters", "screens", "icons", "textures"]
    for part in parts:
        {"monsters": monsters, "screens": screens, "icons": icons, "textures": textures}[part]()
    prune()


if __name__ == "__main__":
    main()
