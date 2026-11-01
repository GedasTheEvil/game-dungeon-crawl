"""Re-bake a model's textures into a temp dir, check them and its .md3 files against the committed ones, and copy only
what is safe (plan metallic-shading). A plain --export rewrites every file: Cycles noise churns textures that did not
change, and uv_unwrap may lay out UVs that no longer fit the committed .md3 (or the script's geometry may have moved
on since the last export).

    python3 tools/blender/rebake.py <model.py> [--only a,b]                    # build(bake, tex_dir) + export(models_dir)
    python3 tools/blender/rebake.py <model.py> --palette COL_SOBEK --stem crocodile_sobek [--ao 0.12]  # a boss texture
    python3 tools/blender/rebake.py <model.py> --tex-path mummy_priest         # build(bake, tex_path): a monster's own
    ... [--apply] [--rgb] [--with-models]

Per texture: the committed file of the same name (media/textures/*/), the RGB drift (largest channel step, share of
pixels off by more than 8), the metal share now and before (alpha < 255). Per .md3 (static scripts only): the corner
positions and UVs against the committed one. --apply copies the textures whose metal mask changed, --rgb also those
whose RGB changed. When an .md3's UVs changed (positions the same), every texture of the run fits the new UVs, not the
committed ones: --apply then refuses unless --with-models, which copies those .md3 files and all the textures. An .md3
whose positions changed is never copied: the script's geometry differs from the game's, export it on purpose.
Blender: $BLENDER, else the Blender 5 of AGENTS.md. Needs numpy and PIL.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
BLENDER = os.environ.get("BLENDER", "/home/gediminas.skucas/Apps/blender-5.2.2-linux-x64/blender")
NOISE = 8  # an RGB step this small is Cycles noise


def inner(opts):
    """Inside Blender: bake (and export) the model into opts' temp dirs."""
    import bpy

    p = os.path.abspath(opts["model"])
    g = {"__file__": p, "__name__": os.path.splitext(os.path.basename(p))[0]}
    exec(open(p).read(), g)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    tex, models = opts["tex"], opts["models"]
    if opts.get("palette"):
        import common

        obj, _ = g["build"](bake=False)
        g["materials"](g[opts["palette"]])
        stem = opts["stem"]
        common.bake_texture(obj, os.path.join(tex, stem + ".png"), g["TEX_SIZE"], stem, ao_distance=opts["ao"])
    elif opts.get("tex_path"):
        g["build"](bake=True, tex_path=os.path.join(tex, opts["tex_path"] + ".png"))
    else:
        kw = {"only": opts["only"]} if opts.get("only") else {}
        objs = g["build"](bake=True, tex_dir=tex, **kw)
        g["export"](objs, models_dir=models)


def committed(kind, name):
    """The committed file of that name under media/<kind>/<category>/, or None."""
    root = os.path.join(REPO, "media", kind)
    hits = [os.path.join(root, d, name) for d in sorted(os.listdir(root)) if os.path.isfile(os.path.join(root, d, name))]
    return hits[0] if len(hits) == 1 else None


def compare_texture(new, old):
    import numpy as np
    from PIL import Image

    a = np.array(Image.open(new).convert("RGBA")).astype(int)
    b = np.array(Image.open(old).convert("RGBA")).astype(int)
    if a.shape != b.shape:
        return {"size": True, "mask": True, "rgb": True}
    d = np.abs(a[..., :3] - b[..., :3]).max(-1)
    return {
        "drift": int(d.max()),
        "off": float((d > NOISE).mean()),
        "metal": float((a[..., 3] < 255).mean()),
        "metal_before": float((b[..., 3] < 255).mean()),
        "mask": bool((a[..., 3] != b[..., 3]).any()),
        "rgb": bool(d.max() > NOISE),
    }


def compare_md3(new, old):
    sys.path.insert(0, HERE)
    from md3 import read_md3

    A, B = read_md3(new), read_md3(old)
    if len(A[0]) != len(B[0]) or len(A[0][0]) != len(B[0][0]):
        return {"moved": True, "uv": True, "why": "frames or triangles differ"}
    step = lambda xs, ys: max((abs(x - y) for p, q in zip(xs, ys) for x, y in zip(p, q)), default=0.0)  # noqa: E731
    moved = max(step(fa, fb) for fa, fb in zip(A[0], B[0]))
    uv = step(A[2], B[2])
    return {"moved": moved > 1e-6, "uv": uv > 1e-6, "why": "corner move {:.4f}, uv step {:.4f}".format(moved, uv)}


def main(args):
    if not args or args[0].startswith("--"):
        sys.exit(__doc__)
    flag = lambda name: args[args.index(name) + 1] if name in args else None  # noqa: E731
    with tempfile.TemporaryDirectory(prefix="rebake_") as tmp:
        opts = {
            "model": args[0],
            "tex": os.path.join(tmp, "textures"),
            "models": os.path.join(tmp, "models"),
            "only": flag("--only").split(",") if flag("--only") else None,
            "palette": flag("--palette"),
            "stem": flag("--stem"),
            "ao": float(flag("--ao") or 0.12),
            "tex_path": flag("--tex-path"),
        }
        if opts["palette"] and not opts["stem"]:
            sys.exit("--palette needs --stem (the texture's file name)")
        os.makedirs(opts["tex"])
        os.makedirs(opts["models"])
        run = subprocess.run(
            [BLENDER, "-b", "--python", os.path.abspath(__file__), "--", "--inner", json.dumps(opts)],
            capture_output=True,
            text=True,
        )
        if run.returncode != 0 or "Traceback" in run.stderr + run.stdout:
            sys.exit(run.stdout[-3000:] + run.stderr[-3000:] + "\nrebake: the Blender run failed")

        models, uv_changed = [], False
        for f in sorted(os.listdir(opts["models"])):
            old = committed("models", f)
            if old is None:
                print("{}: new, not in media/models (not copied)".format(f))
                continue
            c = compare_md3(os.path.join(opts["models"], f), old)
            state = "GEOMETRY CHANGED (not copied)" if c["moved"] else "UVs changed" if c["uv"] else "same"
            print("{}: {} ({})".format(f, state, c["why"]))
            if c["uv"] and not c["moved"]:
                uv_changed = True
                models.append((os.path.join(opts["models"], f), old))
        textures = []
        for f in sorted(os.listdir(opts["tex"])):
            old = committed("textures", f)
            if old is None:
                print("{}: new, not in media/textures (not copied)".format(f))
                continue
            c = compare_texture(os.path.join(opts["tex"], f), old)
            if c.get("size"):
                print("{}: size differs".format(f))
            else:
                print("{}: rgb drift {} ({:.1%} off), metal {:.3f} (before {:.3f}){}".format(
                    f, c["drift"], c["off"], c["metal"], c["metal_before"], ", mask changed" if c["mask"] else ""))
            if uv_changed or c["mask"] or ("--rgb" in args and c["rgb"]):
                textures.append((os.path.join(opts["tex"], f), old))

        if "--apply" not in args:
            print("(dry run: --apply copies)")
            return
        if uv_changed and "--with-models" not in args:
            sys.exit("UVs changed: the textures fit the new .md3 files only; --with-models copies both")
        for src, dst in models + textures:
            shutil.copy(src, dst)
            print("copied", os.path.relpath(dst, REPO))


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    if argv[:1] == ["--inner"]:
        inner(json.loads(argv[1]))
    else:
        main(argv)
