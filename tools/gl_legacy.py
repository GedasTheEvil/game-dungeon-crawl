#!/usr/bin/env python3
"""The legacy-GL counter (plan gl-core-foundation): counts the fixed-function GL a core profile does not have, per file.

  tools/gl_legacy.py check [--base REF]   fails when a file's count grew since REF (default: the merge base of HEAD
                                          and $TIDY_BASE or master); `make layers` (so make tidy, make check and
                                          make tidy-push) runs it
  tools/gl_legacy.py report [PATH...]     the count per file, most first, and the total
  tools/gl_legacy.py show PATH...         each counted line of the files

No baseline file: the counts at the base commit are the baseline, so parallel branches that port different files
never conflict over it. A ported file drops to zero and stays there (a count can only fall); a new file starts at
zero; a renamed one (git's rename detection) keeps its old count; the bridge, src/render/gl_legacy.cpp, is not
checked. Plan gl-core-profile turns it into a ban (every
count zero). Counted: immediate mode, the matrix stack and its readbacks, GLU, display lists, client arrays,
attribute push/pop, fixed-function light, fog, alpha test and texture enable, quads and polygons, and the GLSL 1.x
built-ins in shader sources. Code only: // comments are left out.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIRS = ('src', 'tools/editor', 'tools/model-viewer')
SUFFIXES = ('.cpp', '.h')
# The bridge between ported and legacy drawing (src/render/gl_legacy.h): legacy GL on purpose, deleted by plan
# gl-core-profile with the last legacy caller.
BRIDGE = {'src/render/gl_legacy.cpp'}

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
    # GLSL 1.x and the compatibility built-ins
    r'|gl_Vertex|gl_Color|gl_Normal|gl_MultiTexCoord[0-7]|gl_TexCoord|gl_FrontColor|gl_FragColor|gl_FragData'
    r'|gl_ModelViewMatrix|gl_ProjectionMatrix|gl_ModelViewProjectionMatrix|gl_NormalMatrix|gl_TextureMatrix'
    r'|ftransform|texture2D'
    r')\b'
    r'|\bgl(?:Enable|Disable)\(GL_TEXTURE_2D\)'
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


def default_base():
    target = os.environ.get('TIDY_BASE') or 'master'
    r = git('merge-base', 'HEAD', target)
    return r.stdout.strip() if r.returncode == 0 and r.stdout.strip() else 'HEAD'


def renames(base):
    """new path -> old path, for the files git sees as renamed since base."""
    out = git('diff', '-M', '--name-status', base, '--', *DIRS).stdout
    moved = {}
    for line in out.splitlines():
        parts = line.split('\t')
        if parts[0].startswith('R') and len(parts) == 3:
            moved[parts[2]] = parts[1]
    return moved


def base_count(base, path):
    r = git('show', f'{base}:{path}')
    return count(r.stdout) if r.returncode == 0 else 0


def check(base):
    moved = renames(base)
    grown = []
    for path in tracked_paths():
        if path in BRIDGE:
            continue
        now = count(read(path))
        if now == 0:
            continue
        before = base_count(base, moved.get(path, path))
        if now > before:
            grown.append((path, before, now))
    if grown:
        print(f'gl-legacy: legacy GL grew since {base[:7]} (docs/development.md, GL core path):')
        for path, before, now in grown:
            print(f'  {path}: {before} -> {now}')
        print('  Draw it with Batch, gMv and the default shaders; tools/gl_legacy.py show <file> lists the calls.')
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
        base = rest[rest.index('--base') + 1] if '--base' in rest else default_base()
        return check(base)
    if cmd == 'report':
        return report(rest)
    if cmd == 'show':
        return show(rest)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
