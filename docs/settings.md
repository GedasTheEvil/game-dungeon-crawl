# Settings file

The Options choices are kept in `saves/settings.ini`, next to the save games. The game writes it when a choice
changes in Options (and on quit, when the window was resized). You can also edit it by hand while the game is closed.

* Without the file, everything is at its default. The file is written on the first change.
* The game rewrites the whole file: every key in a fixed order, with a comment before each section. Values you edit
  by hand are kept (the game reads them in). Your own comments and unknown keys are lost.
* A bad value logs a warning to `game.log` and keeps the default.
* An older `saves/settings.txt` (motion effects only) is read once, then replaced by the ini.
* Scenario tests never read or write the file: they always run with the defaults.

Code: `src/state/settings_ini.cpp` (the `Settings` struct, the parser and the writer, GL-free, unit tested in
`tests/unit/settings_test.cpp`), `src/state/settings.cpp` (the file), `src/input/bindings.cpp` (the key bindings).

## Format

`[section]`, `key = value`, comment lines start with `;` or `#`. Section and key names are not case-sensitive.
Booleans are `on` / `off` (`1` / `0`, `true` / `false`, `yes` / `no` read as well).

```ini
[display]
window_width = 800      ; 320-7680, the window when not fullscreen
window_height = 500     ; 200-4320
fullscreen = off

[graphics]
motion_effects = on     ; sprint blur, FOV kick, vignette
toon = off              ; cel bands and ink outlines (F1)
blood = on              ; blood splashes
light_flicker = on      ; torches, braziers and lamps flicker

[sound]
music_volume = 80       ; 0-100
effects_volume = 100    ; 0-100

[controls]
move_left = a, left
...
```

(The `;` comments after the values above are for this page only. The parser reads comments on their own lines.)

## Controls

Each action takes up to two keys and one mouse button, separated by commas. `none` leaves the action unbound. The
same input on two actions is a warning: the first action in the list below keeps it.

| Key | Default | Action |
|---|---|---|
| `move_left` | `a, left` | Move left |
| `move_right` | `d, right` | Move right |
| `climb_up` | `w, up` | Climb up a ladder |
| `climb_down` | `s, down` | Climb down a ladder |
| `jump` | `space, mouse_right` | Jump |
| `sprint` | `shift, right_shift` | Sprint while held |
| `attack` | `v, enter, mouse_left` | Attack |
| `interact` | `e, f12, mouse_middle` | Pick up, pull a lever, answer a riddle |
| `look_left` | `home` | Turn the camera |
| `look_right` | `end` | |
| `look_up` | `page_up` | |
| `look_down` | `page_down` | |
| `equip_club` | `1` | Take the club |
| `equip_sword` | `2` | Take the sword |
| `equip_spear` | `3` | Take the spear |
| `equip_bow` | `4` | Take the bow |
| `quick_heal` | `h` | Drink a healing potion |
| `quick_stamina` | `0` | Drink a stamina potion |
| `inventory` | `i` | Open / close the inventory |
| `map` | `m` | Open / close the draft map |
| `journal` | `j` | Open / close the journal |

Fixed, not in the file: `Esc` (menu / back, so a bad mapping can always be undone), `F1` (toon shading) to `F11`
(`F3`: hitboxes), and mouse movement (looks around). The inventory and the journal move their selection with
`WASD` and the arrow keys, whatever the bindings say.

Key names:

* A character key is the character itself: `a`, `1`, `[`, `/`. Letters match in both cases.
* Named keys: `space`, `enter`, `tab`, `backspace`, `delete`, `comma`, `semicolon`, `hash`, `equals`.
* Special keys: `left`, `right`, `up`, `down`, `page_up`, `page_down`, `home`, `end`, `insert`, `num_lock`,
  `begin`, `keypad_delete`, `shift`, `right_shift`, `ctrl`, `right_ctrl`, `alt`, `right_alt`, `f12`.
* Mouse buttons: `mouse_left`, `mouse_middle`, `mouse_right`.

In Options > Controls, click a cell, then press the key (or click the mouse button, in the mouse column). A key
bound elsewhere moves to the new action. `Delete` clears the cell, `Esc` cancels.
