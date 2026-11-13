# Settings file

The Options choices are kept in `saves/settings.ini`, next to the save games. The game writes it when a choice
changes in Options (and on quit, when the window was resized). You can also edit it by hand while the game is closed.

* Without the file, everything is at its default. The file is written on the first change.
* The game rewrites the whole file: every key in a fixed order, with a comment before each section. Values you edit
  by hand are kept (the game reads them in). Your own comments and unknown keys are lost.
* A bad value logs a warning to `game.log` and keeps the default.
* An older `saves/settings.txt` (motion effects only) is read once, then replaced by the ini.
* Scenario tests never read or write the file: they always run with the defaults.

Code: `src/level/settings_ini.*` (the `Settings` struct, the parser and the writer, GL-free, unit tested in
`tests/unit/settings_test.cpp`), `src/state/settings.cpp` (the file), `src/level/bindings.cpp` (the key bindings).

## Format

`[section]`, `key = value`, comment lines start with `;` or `#`. Section and key names are not case-sensitive.
Booleans are `on` / `off` (`1` / `0`, `true` / `false`, `yes` / `no` read as well).

```ini
[display]
window_width = 800      ; 320-7680, the window when not fullscreen
window_height = 500     ; 200-4320
fullscreen = off
animation_rate = 60     ; 0-240 model poses a second, blended between the key frames; 0: the key frames only
frame_rate = 0          ; -1-240 frames a second drawn; 0: the monitor's (vsync), -1: unlimited
brightness = 100        ; 70-150 percent, a gamma over the whole picture (menus and HUD too)

[graphics]
motion_effects = on     ; sprint blur, FOV kick, vignette
toon = off              ; cel bands and ink outlines (F1)
blood = on              ; blood splashes
light_flicker = on      ; torches, braziers and lamps flicker
cut_scenes = on         ; the view goes to a far gate a lever or a boss's death opens
camera_recenter = on    ; a look-key peek eases back to straight ahead when left alone (a mouse turn stays)

[mouse]
sensitivity = 100       ; 20-300 percent, how far the view turns per mouse movement
invert_y = off          ; on: moving the mouse up looks down

[gameplay]
hints = on              ; the first hints: how to jump, climb, attack, ... the first time it is possible in a run

[sound]
master_volume = 100     ; 0-100, scales the music and the effects
music_volume = 80       ; 0-100
effects_volume = 100    ; 0-100
mute = off              ; silences both, the volumes kept

[controls]
move_left = a, left
...
```

(The `;` comments after the values above are for this page only. The parser reads comments on their own lines.)

Options shows these on four tabs: Controls, Mouse (sensitivity, invert Y, camera recentre), Display (window size,
fullscreen, brightness, the rest of `[display]` and `[graphics]`) and Sound. The Brightness row has a calibration box:
a black square with an ankh a shade above black; set the brightness so the ankh is barely visible. The Window size row
steps through the common sizes that fit the desktop, then the desktop's own size
([settings-extras](plan/solved/settings-extras.feature.md)).

## Controls

Each action takes up to two keys and one mouse button, separated by commas. `none` leaves the action unbound. The
same input on two actions is a warning: the first action in the list below keeps it.

| Key | Default | Action |
|---|---|---|
| `move_left` | `a, left` | Move left |
| `move_right` | `d, right` | Move right |
| `climb_up` | `w, up` | Climb up a ladder |
| `climb_down` | `s, down` | Climb down a ladder; off a ladder, crouch while held: under a crawl space, ducking high swings and shots ([plan](plan/solved/crouch.md)) |
| `jump` | `space, mouse_right` | Jump |
| `sprint` | `shift, right_shift` | Sprint while held |
| `attack` | `v, enter, mouse_left` | Attack |
| `interact` | `e, f12, mouse_middle` | Pick up, pull a lever, answer a riddle |
| `drag` | `r` | Take hold of a mirror in the next cell, let go ([mirror-drag](plan/mirror-drag.feature.md)) |
| `look_left` | `home` | Turn the camera while held (the mouse turns it too); with `camera_recenter` on, a look-key peek eases back once left alone (a mouse turn stays) |
| `look_right` | `end` | |
| `look_up` | `page_up` | |
| `look_down` | `page_down` | |
| `equip_melee` | `1` | Take the next melee weapon held, in inventory order |
| `equip_ranged` | `2` | Take the next ranged weapon held |
| `quick_heal` | `3, h` | Drink a healing potion |
| `quick_stamina` | `4, 0` | Drink a stamina potion |
| `quick_antidote` | `5, equals` | Drink an antidote (only while poisoned) |
| `stamina_skills` | `q` | Switch the learnt stamina skills on / off (flaming arrows) |
| `inventory` | `i` | Open / close the inventory |
| `map` | `m` | Open / close the draft map |
| `journal` | `j` | Open / close the journal |
| `skills` | `k` | Open / close the Heka screen (skills) |

Fixed, not in the file: `Esc` (menu / back, so a bad mapping can always be undone), `F1` (toon shading) to `F11`
(`F5`: quick save, `F9`: quick load; `F3` shows the hitboxes only with `bin/game --debug`), and mouse movement (looks around). The menus, the inventory, the Heka screen and the journal
move their focus or selection with the arrow keys and the keys bound to move left / right and climb up / down; Enter,
Space or the interact key picks ([keyboard-menus](plan/solved/keyboard-menus.md)). Left / Right on an Options row
change it. The inventory's `U` (upgrade) and `Tab` (next tab) are fixed. On the draft map the wheel and `+` / `-` zoom,
the arrows (and the move keys) and a drag pan ([map-symbols-and-zoom](plan/solved/map-symbols-and-zoom.md)).

Key names:

* A character key is the character itself: `a`, `1`, `[`, `/`. Letters match in both cases.
* Named keys: `space`, `enter`, `tab`, `backspace`, `delete`, `comma`, `semicolon`, `hash`, `equals`.
* Special keys: `left`, `right`, `up`, `down`, `page_up`, `page_down`, `home`, `end`, `insert`, `num_lock`,
  `begin`, `keypad_delete`, `shift`, `right_shift`, `ctrl`, `right_ctrl`, `alt`, `right_alt`, `f12`.
* Mouse buttons: `mouse_left`, `mouse_middle`, `mouse_right`.

In Options > Controls, click a cell, then press the key (or click the mouse button, in the mouse column). A key
bound elsewhere moves to the new action. `Delete` clears the cell, `Esc` cancels.
