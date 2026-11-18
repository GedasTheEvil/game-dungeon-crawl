#!/usr/bin/env python3
"""The legacy-GL ban (plan gl-core-profile; a counter since plan gl-core-foundation): the game runs in a GL 3.3 core
context, so any fixed-function GL, or GL a core context rejects, is an error.

  tools/gl_legacy.py check               fails on any counted line; `make layers` (so make tidy, make check and
                                         make tidy-push) runs it
  tools/gl_legacy.py report [PATH...]    the count per file, most first, and the total
  tools/gl_legacy.py show PATH...        each counted line of the files

glad is generated for the core profile, so most legacy calls do not compile anyway; this catches the rest: the GLSL
1.x built-ins in shader sources, and what core has but rejects or draws otherwise (glLineWidth above 1,
GL_LINE_SMOOTH, glPolygonMode of one side, glPointSize's sprites; WideLines draws those). Counted: immediate mode,
the matrix stack and its readbacks, GLU, display lists, client arrays, attribute push/pop, fixed-function light, fog,
alpha test and texture enable, quads and polygons, wide and smooth lines, one-sided polygon modes, point sizes, and
the GLSL 1.x built-ins in shader sources. Code only: // comments are left out.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIRS = ('src', 'tools/editor', 'tools/model-viewer')
SUFFIXES = ('.cpp', '.h')

LEGACY = re.compile(
    r'\b(?:'
    # immediate mode, the matrix stack, GLU
    r'glBegin|glEnd|glVertex[234][dfis]v?|glColor[34](?:u?b|[dfis]|u[is])v?|glTexCoord[1234][dfis]v?|glNormal3[dfis]v?'
    r'|glRect[dfis]v?|glRasterPos\w+|glWindowPos\w+|glBitmap|glDrawPixels'
    r'|glPushMatrix|glPopMatrix|glTranslate[df]|glRotate[df]|glScale[df]|glMultMatrix[df]|glLoadMatrix[df]'
    r'|glLoadIdentity|glMatrixMode|glOrtho|glFrustum|glu[A-Z]\w*'
    r'|GL_MODELVIEW_MATRIX|GL_PROJECTION_MATRIX'
    # display lists, client arrays, attribute stacks
    r'|glNewList|glEndList|glCallLists?|glGenLists|glDeleteLists|glListBase'
    r'|glVertexPointer|glColorPointer|glTexCoordPointer|glNormalPointer|glEnableClientState|glDisableClientState'
    r'|glClientActiveTexture|glPushAttrib|glPopAttrib|glPushClientAttrib|glPopClientAttrib'
    # fixed-function state
    r'|glAlphaFunc|glShadeModel|glLight[fi]v?|glLightModel[fi]v?|glMaterial[fi]v?|glColorMaterial|glTexEnv[fi]v?'
    r'|glFog[fi]v?|glLineStipple|glPolygonStipple'
    r'|GL_ALPHA_TEST|GL_LIGHTING|GL_LIGHT[0-7]|GL_NORMALIZE|GL_COLOR_MATERIAL|GL_CLAMP|GL_LINE_STIPPLE'
    r'|GL_QUADS|GL_QUAD_STRIP|GL_POLYGON'
    # in core, but rejected (forward compatible) or not drawn reliably: WideLines draws them
    r'|glLineWidth|GL_LINE_SMOOTH|GL_POLYGON_SMOOTH|GL_POLYGON_OFFSET_LINE|glPointSize'
    # GLSL 1.x and the compatibility built-ins
    r'|gl_Vertex|gl_Color|gl_Normal|gl_MultiTexCoord[0-7]|gl_TexCoord|gl_FrontColor|gl_FragColor|gl_FragData'
    r'|gl_ModelViewMatrix|gl_ProjectionMatrix|gl_ModelViewProjectionMatrix|gl_NormalMatrix|gl_TextureMatrix'
    r'|ftransform|texture2D'
    r')\b'
    r'|\bgl(?:Enable|Disable)\(GL_TEXTURE_2D\)'
    r'|\bglPolygonMode\(GL_(?:FRONT|BACK)\b'
    r'|#version 1[0-9]0\b|#version \d+ compatibility'
    r'|\b(?:varying|attribute)\s+(?:vec[234]|float|mat[234])\b'  # GLSL 1.x declarations, not a C++ name
)
# GL_QUADS etc. above end at a word boundary, so GL_POLYGON_OFFSET_FILL and GL_LIGHT_MODEL_AMBIENT do not count.


def code(text):
    """The text without // comments (a // inside a string literal is rare here; a raw shader's comments go too)."""
    return re.sub(r'//[^\n]*', '', text)


def count(text):
    return len(LEGACY.findall(code(text)))


def git(*args):
    return subprocess.run(['git', *args], cwd=ROOT, capture_output=True, text=True, check=False)


def tracked_paths():
    out = git('ls-files', '--cached', '--others', '--exclude-standard', '--', *DIRS).stdout.split()
    return sorted(p for p in out if p.endswith(SUFFIXES) and os.path.isfile(os.path.join(ROOT, p)))


def read(path):
    with open(os.path.join(ROOT, path), encoding='utf-8', errors='replace') as f:
        return f.read()


def check():
    rows = [(count(read(p)), p) for p in tracked_paths()]
    rows = [r for r in rows if r[0] > 0]
    if rows:
        print('gl-legacy: legacy GL in a core context (docs/development.md, GL core path):')
        for n, path in rows:
            print(f'  {path}: {n}')
        print('  Draw it with Batch, gMv, the default shaders and WideLines; tools/gl_legacy.py show <file> lists the calls.')
        return 1
    return 0


def report(paths):
    rows = [(count(read(p)), p) for p in (paths or tracked_paths())]
    rows = [r for r in rows if r[0] > 0]
    rows.sort(key=lambda r: (-r[0], r[1]))
    for n, p in rows:
        print(f'{n:6d}  {p}')
    print(f'{sum(n for n, _ in rows):6d}  total, {len(rows)} files')
    return 0


def show(paths):
    for p in paths:
        for i, line in enumerate(read(p).splitlines(), 1):
            hits = LEGACY.findall(code(line))
            if hits:
                print(f'{p}:{i}: {", ".join(hits)}')
    return 0


def main(argv):
    if not argv or argv[0] in ('-h', '--help'):
        print(__doc__)
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd == 'check':
        return check()
    if cmd == 'report':
        return report(rest)
    if cmd == 'show':
        return show(rest)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
