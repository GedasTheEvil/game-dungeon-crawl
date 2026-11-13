"""Generate the draft map's symbol sheet media/textures/ui/map_marks.png (RGBA, 8 x 6 cells of 128 px).

    python3 tools/textures/map_marks.py [out.png]

Pencil strokes on clear, drawn at 4x and downsampled (plan map-legend-sprites). Cell order must match MapMark in
src/level/map_marks.h: a mark's sprite is the cell of its number, then from cell 40 on the second looks
(MarkDef::stateCell): the gate open, the boss gate open, the lever pulled, the rock fallen. A sprite holds the map
cell in its middle, MARGIN of its width free on every side (MAP_MARK_MARGIN). Strokes are in map cell units, y up,
as the map drew them in code before the sheet. White sprites (key, gate, lever, sun disc) are tinted by the map with
the lock colour's pencil. Sprites with a way (light shaft, mirror: down; sandstorm: blowing right) are turned by it.

No build step runs this: it writes only when run by hand. Once the PNG is edited by hand, run it to a side file and
copy only the new cells over (docs/ui.md, Draft map).
"""

import math
import os
import sys

from PIL import Image, ImageDraw

from decals import bleed

CELL = 128
SCALE = 4
S = CELL * SCALE  # drawing resolution per sprite
GRID_X, GRID_Y = 8, 6
MARGIN = 0.125  # MAP_MARK_MARGIN
UNIT = S * (1 - 2 * MARGIN)  # one map cell, in drawing pixels
LINE = 0.065  # the pencil's width, in map cells
JITTER = 0.035  # its wobble at both ends of a stroke, in map cells


def rgb(r, g, b):
    return (round(r * 255), round(g * 255), round(b * 255))


WHITE = (255, 255, 255)  # tinted by the map (MarkTint::Lock)
GRAPHITE = rgb(0.22, 0.21, 0.20)
GREY = rgb(0.50, 0.48, 0.45)  # done with: an answered sphinx gate's question mark, a treasure taken
RED = rgb(0.72, 0.14, 0.08)
GOLD = rgb(0.78, 0.56, 0.10)
BLUE = rgb(0.14, 0.36, 0.80)  # the exit's plasma
LAPIS = rgb(0.12, 0.27, 0.62)
GREEN = rgb(0.14, 0.48, 0.18)
PLASMA = rgb(0.10, 0.50, 0.55)  # a teleporter's and a ring platform's
OBSIDIAN = rgb(0.08, 0.06, 0.08)  # the boss gate's
SAND = rgb(0.66, 0.47, 0.20)
INK_FADED = rgb(0.52, 0.40, 0.26)
FLAME = rgb(0.85, 0.38, 0.06)


def noise(seed):
    """Stable per-stroke randomness in [-1, 1] (the map's old hash, so the strokes wobble as they did)."""
    seed &= 0xFFFFFFFF
    seed ^= seed >> 16
    seed = (seed * 0x7FEB352D) & 0xFFFFFFFF
    seed ^= seed >> 15
    seed = (seed * 0x846CA68B) & 0xFFFFFFFF
    seed ^= seed >> 16
    return (seed & 0xFFFF) / 32767.5 - 1


class Sketch:
    """One sprite. Coordinates in map cell units, y up; (0, 0) is the cell's lower left corner."""

    def __init__(self, seed):
        self.img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        self.seed = seed * 7919

    def _px(self, x, y):
        return (S * MARGIN + x * UNIT, S * (1 - MARGIN) - y * UNIT)

    def _stroke(self, x0, y0, x1, y1, colour, alpha):
        layer = Image.new("RGBA", (S, S), colour + (0,))
        d = ImageDraw.Draw(layer)
        fill = colour + (round(alpha * 255),)
        a, b = self._px(x0, y0), self._px(x1, y1)
        w = LINE * UNIT
        d.line([a, b], fill=fill, width=round(w))
        for cx, cy in (a, b):  # round ends
            d.ellipse([cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2], fill=fill)
        self.img = Image.alpha_composite(self.img, layer)

    def pencil(self, x0, y0, x1, y1, colour, alpha, seed, jitter=JITTER):
        """A hand-drawn line: off at both ends, gone over twice with less pressure the second time."""
        for p in range(2):
            k = (self.seed + seed) * 8 + p * 4
            self._stroke(
                x0 + jitter * noise(k),
                y0 + jitter * noise(k + 1),
                x1 + jitter * noise(k + 2),
                y1 + jitter * noise(k + 3),
                colour,
                alpha if p == 0 else alpha * 0.45,
            )

    def loop(self, cx, cy, rx, ry, n, colour, alpha, seed, jitter=JITTER):
        """Closed polygon through n points on an ellipse round (cx, cy)."""
        self.arc(cx, cy, rx, ry, 0, 2 * math.pi, n, colour, alpha, seed, jitter)

    def arc(self, cx, cy, rx, ry, a0, a1, n, colour, alpha, seed, jitter=JITTER):
        for i in range(n):
            u0 = a0 + (a1 - a0) * i / n
            u1 = a0 + (a1 - a0) * (i + 1) / n
            self.pencil(
                cx + rx * math.cos(u0),
                cy + ry * math.sin(u0),
                cx + rx * math.cos(u1),
                cy + ry * math.sin(u1),
                colour,
                alpha,
                seed + i,
                jitter,
            )

    def path(self, pts, colour, alpha, seed, jitter=JITTER):
        for i in range(len(pts) - 1):
            self.pencil(*pts[i], *pts[i + 1], colour, alpha, seed + i, jitter)


def arch(sk, colour, seed=0):
    sk.pencil(0.22, 0, 0.22, 0.55, colour, 0.9, seed)
    sk.pencil(0.78, 0, 0.78, 0.55, colour, 0.9, seed + 1)
    sk.arc(0.5, 0.55, 0.28, 0.3, 0, math.pi, 4, colour, 0.9, seed + 2)


def zigzag(sk, height, colour, seed=0, ceiling=False):
    """Three teeth on the floor; `ceiling`: hanging from the ceiling."""
    for k in range(3):
        left = 0.1 + 0.8 * k / 3
        right = 0.1 + 0.8 * (k + 1) / 3
        tip = (left + right) / 2
        base, top = (0.95, 1 - height) if ceiling else (0.05, height)
        sk.pencil(left, base, tip, top, colour, 0.9, seed + 2 * k)
        sk.pencil(tip, top, right, base, colour, 0.9, seed + 2 * k + 1)


def columns(sk, seed=0):
    """The teleporter's two columns, a capital on each."""
    sk.pencil(0.15, 0, 0.15, 0.9, GRAPHITE, 0.9, seed)
    sk.pencil(0.85, 0, 0.85, 0.9, GRAPHITE, 0.9, seed + 1)
    sk.pencil(0.07, 0.9, 0.23, 0.9, GRAPHITE, 0.9, seed + 2)
    sk.pencil(0.77, 0.9, 0.93, 0.9, GRAPHITE, 0.9, seed + 3)


def gate(sk, colour, is_open, seed=0):
    bottom = 0.75 if is_open else 0.0  # an open gate hangs up in the ceiling
    sk.pencil(0.15, 0.95, 0.85, 0.95, colour, 0.95, seed)
    if not is_open:
        sk.pencil(0.15, 0.05, 0.85, 0.05, colour, 0.95, seed + 1)
    for k in range(3):
        u = 0.3 + 0.2 * k
        sk.pencil(u, bottom, u, 0.95, colour, 0.95, seed + 2 + k)


def portal(sk, colour):
    """The sphinx's arch, the portal in it in its plasma's colour: red in, blue out."""
    arch(sk, GRAPHITE)
    sk.loop(0.5, 0.42, 0.17, 0.3, 8, colour, 0.95, 8)
    sk.loop(0.5, 0.42, 0.07, 0.13, 6, colour, 0.85, 16)


def entrance(sk):
    portal(sk, RED)


def exit_(sk):
    portal(sk, BLUE)


def sphinx_door(sk, colour):
    """A door, a question mark in it."""
    sk.pencil(0.18, 0, 0.18, 0.92, GRAPHITE, 0.9, 0)
    sk.pencil(0.82, 0, 0.82, 0.92, GRAPHITE, 0.9, 1)
    sk.pencil(0.12, 0.92, 0.88, 0.92, GRAPHITE, 0.9, 2)
    sk.path([(0.34, 0.6), (0.42, 0.74), (0.58, 0.76), (0.66, 0.62), (0.5, 0.47), (0.5, 0.32)], colour, 0.95, 8)
    sk.loop(0.5, 0.16, 0.04, 0.04, 5, colour, 0.95, 13)


def sphinx_gate(sk):  # the riddle still open: gold
    sphinx_door(sk, GOLD)


def sphinx_gate_answered(sk):
    sphinx_door(sk, GREY)


def teleporter(sk):
    """The columns, the plasma between them."""
    columns(sk)
    sk.loop(0.5, 0.45, 0.2, 0.32, 8, PLASMA, 0.9, 4)
    sk.loop(0.5, 0.45, 0.08, 0.14, 6, PLASMA, 0.8, 12)


def ring_platform(sk):
    """The platform, three rings stacked over it."""
    sk.pencil(0.1, 0.06, 0.9, 0.06, GRAPHITE, 0.95, 0)
    for k in range(3):
        sk.loop(0.5, 0.3 + 0.22 * k, 0.32, 0.07, 8, PLASMA, 0.9, 1 + 8 * k)


def name_gate(sk):
    """The columns, sealed by a cartouche: the level's name opens it."""
    columns(sk)
    sk.loop(0.5, 0.5, 0.13, 0.26, 7, GOLD, 0.95, 4)
    sk.pencil(0.33, 0.2, 0.67, 0.2, GOLD, 0.95, 11)


def ladder(sk):
    sk.pencil(0.3, 0, 0.3, 1, GRAPHITE, 0.8, 0)
    sk.pencil(0.7, 0, 0.7, 1, GRAPHITE, 0.8, 1)
    for k in range(3):
        v = 0.2 + 0.3 * k
        sk.pencil(0.3, v, 0.7, v, GRAPHITE, 0.8, 2 + k)


def cross(sk, colour, alpha):
    sk.pencil(0.25, 0.2, 0.75, 0.7, colour, alpha, 0)
    sk.pencil(0.25, 0.7, 0.75, 0.2, colour, alpha, 1)


def treasure(sk):  # X marks the spot
    cross(sk, RED, 0.95)


def treasure_taken(sk):  # the X gone pale
    cross(sk, GREY, 0.7)


def key(sk):
    sk.loop(0.3, 0.5, 0.14, 0.14, 6, WHITE, 0.95, 0)
    sk.pencil(0.44, 0.5, 0.85, 0.5, WHITE, 0.95, 6)
    sk.pencil(0.72, 0.5, 0.72, 0.35, WHITE, 0.95, 7)
    sk.pencil(0.84, 0.5, 0.84, 0.35, WHITE, 0.95, 8)


def gate_closed(sk):
    gate(sk, WHITE, False)


def gate_open(sk):
    gate(sk, WHITE, True)


def boss_gate(sk):
    """The gate in obsidian, a seal on it while closed: the boss's death breaks it."""
    gate(sk, OBSIDIAN, False)
    sk.loop(0.5, 0.5, 0.17, 0.17, 4, RED, 0.95, 5)


def boss_gate_open(sk):
    gate(sk, OBSIDIAN, True)


def lever_at(sk, tip_x):
    sk.pencil(0.25, 0.08, 0.75, 0.08, WHITE, 0.95, 0)
    sk.pencil(0.5, 0.08, tip_x, 0.55, WHITE, 0.95, 1)
    sk.loop(tip_x, 0.6, 0.07, 0.07, 5, WHITE, 0.95, 2)


def lever(sk):
    lever_at(sk, 0.75)


def lever_pulled(sk):
    lever_at(sk, 0.25)


def spikes(sk):
    zigzag(sk, 0.4, GRAPHITE)


def death_trap(sk):
    zigzag(sk, 0.65, RED)


def ceiling_spikes(sk):
    zigzag(sk, 0.4, GRAPHITE, ceiling=True)


def ceiling_death(sk):
    zigzag(sk, 0.65, RED, ceiling=True)


def rock_fall(sk):  # loose ceiling: a crack
    sk.path([(0.15, 0.95), (0.35, 0.78), (0.55, 0.9), (0.8, 0.72)], GRAPHITE, 0.9, 0)


def rock_fallen(sk):  # a boulder on the floor
    sk.loop(0.5, 0.25, 0.28, 0.2, 7, GRAPHITE, 0.9, 0)


def pressure_plate(sk):
    """A slab in the floor with a gem on it (plan pressure-plate); white, tinted with its lock colour."""
    sk.pencil(0.18, 0.08, 0.82, 0.08, WHITE, 0.95, 0)
    sk.pencil(0.18, 0.18, 0.82, 0.18, WHITE, 0.95, 1)
    sk.pencil(0.18, 0.08, 0.18, 0.18, WHITE, 0.9, 2)
    sk.pencil(0.82, 0.08, 0.82, 0.18, WHITE, 0.9, 3)
    sk.loop(0.5, 0.3, 0.09, 0.08, 6, WHITE, 0.95, 4, 0.02)


def stone(sk):
    """A squared stone block on the floor, to drag (plan pressure-plate)."""
    sk.path([(0.26, 0.06), (0.74, 0.06), (0.76, 0.52), (0.24, 0.5), (0.26, 0.06)], GRAPHITE, 0.95, 0)
    sk.pencil(0.3, 0.3, 0.46, 0.34, GRAPHITE, 0.6, 7)


def dart_plate(sk):
    """A slab in the floor, the three dart holes in the wall above it."""
    sk.pencil(0.2, 0.08, 0.8, 0.08, GRAPHITE, 0.95, 0)
    sk.pencil(0.2, 0.16, 0.8, 0.16, GRAPHITE, 0.95, 1)
    for k in range(3):
        sk.loop(0.28 + 0.22 * k, 0.4, 0.06, 0.06, 5, GRAPHITE, 0.9, 2 + 8 * k, 0.02)


def wall_spikes(sk):
    """The slots in the back wall the spikes come out of, a 3 x 3 grid."""
    for c in range(3):
        for r in range(3):
            sk.loop(0.25 + 0.25 * c, 0.22 + 0.25 * r, 0.05, 0.05, 5, GRAPHITE, 0.9, 8 * (c * 3 + r), 0.02)


def fire_vent(sk):
    """The vent's mouth low in the back wall, a tongue of flame over it."""
    sk.pencil(0.3, 0.12, 0.7, 0.12, GRAPHITE, 0.95, 0)
    sk.pencil(0.3, 0.22, 0.7, 0.22, GRAPHITE, 0.95, 1)
    sk.path([(0.5, 0.28), (0.42, 0.55), (0.55, 0.8)], GRAPHITE, 0.9, 2)


def poison_drip(sk):
    """The puddle on the floor, the drop hanging from the ceiling over it, its fall."""
    sk.loop(0.5, 0.1, 0.32, 0.06, 7, GRAPHITE, 0.95, 0)
    sk.loop(0.5, 0.84, 0.07, 0.09, 5, GRAPHITE, 0.9, 8)
    sk.pencil(0.5, 0.66, 0.5, 0.5, GRAPHITE, 0.8, 16)
    sk.pencil(0.5, 0.4, 0.5, 0.24, GRAPHITE, 0.8, 17)


def sand_gust(sk):
    """Three lines of wind blowing right, an arrowhead on the middle one."""
    for k in range(3):
        v = 0.25 + 0.25 * k
        tail = 0.18 * (k % 2)
        sk.pencil(0.5 - (0.38 - tail), v, 0.88, v, SAND, 0.9, k)
    sk.pencil(0.88, 0.5, 0.72, 0.62, SAND, 0.9, 4)
    sk.pencil(0.88, 0.5, 0.72, 0.38, SAND, 0.9, 5)


def light_shaft(sk):
    """Rays fanning out of the crack, the beam going down."""
    for k in (-1, 0, 1):
        sk.pencil(0.5, 0.9, 0.5 + 0.18 * k, 0.2, GOLD, 0.9, k + 1)


def mirror(sk):
    """The disc on its post, a stroke the way it points: down. The whole sprite turns with the mirror."""
    sk.pencil(0.5, 0.95, 0.5, 0.65, GRAPHITE, 0.9, 0)
    sk.loop(0.5, 0.5, 0.15, 0.15, 7, GRAPHITE, 0.9, 1)
    sk.pencil(0.5, 0.5, 0.5, 0.1, GOLD, 0.95, 9)


def sun_disc(sk):
    """A disc with rays, in its lock colour."""
    sk.loop(0.5, 0.55, 0.16, 0.16, 7, WHITE, 0.95, 0)
    for k in range(6):
        a = 2 * math.pi * k / 6
        sk.pencil(
            0.5 + 0.22 * math.cos(a),
            0.55 + 0.22 * math.sin(a),
            0.5 + 0.34 * math.cos(a),
            0.55 + 0.34 * math.sin(a),
            WHITE,
            0.9,
            8 + k,
        )


def inscription(sk):
    """A cartouche on the back wall: an upright oval on its bar."""
    sk.loop(0.5, 0.55, 0.14, 0.24, 7, GRAPHITE, 0.9, 0)
    sk.pencil(0.32, 0.28, 0.68, 0.28, GRAPHITE, 0.95, 8)


def page(sk):
    """A page of the earlier expedition: a sheet with lines of writing."""
    sk.path([(0.3, 0.12), (0.3, 0.82), (0.72, 0.82), (0.72, 0.12), (0.3, 0.12)], GRAPHITE, 0.9, 0)
    for k in range(3):
        v = 0.32 + 0.17 * k
        sk.pencil(0.38, v, 0.64, v, INK_FADED, 0.9, 4 + k)


def ankh(sk):
    sk.loop(0.5, 0.74, 0.12, 0.16, 7, GOLD, 0.95, 0)
    sk.pencil(0.28, 0.55, 0.72, 0.55, GOLD, 0.95, 7)
    sk.pencil(0.5, 0.58, 0.5, 0.08, GOLD, 0.95, 8)


def naos(sk):
    """A shrine: the naos's walls on their base, the cornice flaring out over them."""
    sk.pencil(0.1, 0.04, 0.9, 0.04, GRAPHITE, 0.95, 0)
    sk.pencil(0.2, 0.04, 0.2, 0.78, GRAPHITE, 0.9, 1)
    sk.pencil(0.8, 0.04, 0.8, 0.78, GRAPHITE, 0.9, 2)
    sk.path([(0.2, 0.78), (0.1, 0.94), (0.9, 0.94), (0.8, 0.78), (0.2, 0.78)], GRAPHITE, 0.9, 3, 0.025)


def shrine_isis(sk):  # her sign, the throne: a stepped seat
    naos(sk)
    sk.path([(0.36, 0.16), (0.36, 0.66), (0.48, 0.66), (0.48, 0.4), (0.66, 0.4), (0.66, 0.16)], LAPIS, 0.95, 16, 0.025)


def shrine_montu(sk):  # the war god's sun disc under two plumes
    naos(sk)
    sk.loop(0.5, 0.3, 0.12, 0.12, 6, RED, 0.95, 16, 0.025)
    sk.pencil(0.44, 0.44, 0.42, 0.7, RED, 0.95, 24, 0.025)
    sk.pencil(0.56, 0.44, 0.58, 0.7, RED, 0.95, 25, 0.025)


def shrine_ptah(sk):  # his sceptre: a staff with the djed's crossbars
    naos(sk)
    sk.pencil(0.5, 0.12, 0.5, 0.7, GREEN, 0.95, 16, 0.025)
    for k in range(3):
        v = 0.46 + 0.09 * k
        sk.pencil(0.39, v, 0.61, v, GREEN, 0.95, 17 + k, 0.02)


def shrine_amun(sk):  # his crown's two tall plumes
    naos(sk)
    sk.loop(0.42, 0.42, 0.06, 0.26, 6, GOLD, 0.95, 16, 0.02)
    sk.loop(0.58, 0.42, 0.06, 0.26, 6, GOLD, 0.95, 24, 0.02)


def shrine_serket(sk):  # her scorpion: the body, the tail curled over it
    naos(sk)
    sk.loop(0.46, 0.26, 0.13, 0.09, 6, PLASMA, 0.95, 16, 0.02)
    sk.arc(0.52, 0.48, 0.14, 0.18, -0.4 * math.pi, 0.75 * math.pi, 5, PLASMA, 0.95, 24, 0.02)


def stargate(sk):
    """The ring standing on its base, the chevrons round it."""
    sk.pencil(0.2, 0.03, 0.8, 0.03, GRAPHITE, 0.95, 0)
    sk.loop(0.5, 0.52, 0.44, 0.44, 12, GRAPHITE, 0.9, 1, 0.02)
    sk.loop(0.5, 0.52, 0.3, 0.3, 10, GRAPHITE, 0.9, 16, 0.02)
    for k in range(7):
        a = math.pi / 2 + 2 * math.pi * k / 7
        sk.pencil(
            0.5 + 0.31 * math.cos(a),
            0.52 + 0.31 * math.sin(a),
            0.5 + 0.43 * math.cos(a),
            0.52 + 0.43 * math.sin(a),
            GOLD,
            0.95,
            32 + k,
            0.02,
        )


def dial(sk):
    """The dial device: a pedestal, the dome on it, the red crystal in its middle."""
    sk.path([(0.3, 0.04), (0.38, 0.42), (0.62, 0.42), (0.7, 0.04), (0.3, 0.04)], GRAPHITE, 0.9, 0, 0.025)
    sk.arc(0.5, 0.42, 0.3, 0.3, 0, math.pi, 6, GRAPHITE, 0.9, 8, 0.025)
    sk.pencil(0.18, 0.42, 0.82, 0.42, GRAPHITE, 0.9, 16, 0.025)
    sk.loop(0.5, 0.54, 0.07, 0.07, 5, RED, 0.95, 17, 0.02)


def brazier(sk):
    """A bowl on three legs, the fire in it."""
    sk.arc(0.5, 0.5, 0.3, 0.16, math.pi, 2 * math.pi, 5, GRAPHITE, 0.9, 0, 0.025)
    sk.pencil(0.18, 0.5, 0.82, 0.5, GRAPHITE, 0.95, 8, 0.025)
    sk.pencil(0.36, 0.36, 0.26, 0.03, GRAPHITE, 0.9, 9)
    sk.pencil(0.64, 0.36, 0.74, 0.03, GRAPHITE, 0.9, 10)
    sk.pencil(0.5, 0.34, 0.5, 0.03, GRAPHITE, 0.9, 11)
    sk.path([(0.36, 0.56), (0.42, 0.8), (0.5, 0.64), (0.54, 0.95), (0.62, 0.7), (0.66, 0.56)], FLAME, 0.95, 16, 0.025)


# In MapMark order (src/level/map_marks.h), then the second looks.
MARKS = [
    entrance,
    exit_,
    sphinx_gate,
    sphinx_gate_answered,
    teleporter,
    ring_platform,
    name_gate,
    ladder,
    treasure,
    treasure_taken,
    key,
    gate_closed,
    boss_gate,
    lever,
    spikes,
    death_trap,
    ceiling_spikes,
    ceiling_death,
    rock_fall,
    dart_plate,
    wall_spikes,
    fire_vent,
    poison_drip,
    sand_gust,
    light_shaft,
    mirror,
    sun_disc,
    inscription,
    page,
    ankh,
    shrine_isis,
    shrine_montu,
    shrine_ptah,
    shrine_amun,
    shrine_serket,
    stargate,
    dial,
    brazier,
    pressure_plate,
    stone,
    gate_open,  # 40
    boss_gate_open,
    lever_pulled,
    rock_fallen,
]


def main(path):
    atlas = Image.new("RGBA", (CELL * GRID_X, CELL * GRID_Y), (0, 0, 0, 0))
    for k, draw in enumerate(MARKS):
        sk = Sketch(k + 1)
        draw(sk)
        cell = sk.img.resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(cell, ((k % GRID_X) * CELL, (k // GRID_X) * CELL))
    bleed(atlas).save(path)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "media", "textures", "ui", "map_marks.png"))
