## Process
After writing the code, verify it compiles by running `make`.
Once in compiles, run the code style checks `make format` and `make tidy`.

Never commit to git directly.

## Blender
Always use Blender 5: `/home/gediminas.skucas/Apps/blender-5.2.2-linux-x64/blender`.
Never use the system `blender` (v4.0).
The `blender` MCP server needs Blender 5 running with the MCP add-on started (port 9876).

## Model remodelling
Models are rebuilt procedurally in Blender Python scripts. Tools, format conventions and per-model status: [docs/remodeling.md](docs/remodeling.md).

## Command whitelist
These commands do need confirmation to run
* make tidy
* make tidy-fix
* make format
* make clean
* make
