#ifndef GAMEPLAY_MONSTER_KINDS_H
#define GAMEPLAY_MONSTER_KINDS_H

// The Mother of Webs (plan camel-spider-boss, MonsterKind::webs, douseCells): every MOTHER_WEB_EVERY_MS she spins a web
// over the floor of the cell she stands in, up to MOTHER_WEB_MAX at a time. A player standing in a web walks at the
// sand fall's slow; a blade's swing cuts it, fire burns it (gone after WEB_BURN_MS of flames). She puts out every wall
// torch and oil lamp up to MOTHER_DOUSE_CELLS along her row for good (a puff of smoke for SNUFF_MS); the braziers stay
// lit.
constexpr int MOTHER_WEB_EVERY_MS = 1200;
constexpr int MOTHER_WEB_MAX = 8;
constexpr int MOTHER_DOUSE_CELLS = 2;
constexpr int WEB_BURN_MS = 700;
constexpr int WEB_CUT_MS = 250;
constexpr int SNUFF_MS = 900;
// Babi (plan babi-boss, VolleyRules): his big coal sets the floor of a cell burning (Hazards::Blaze). Whoever stands
// in it, not in a jump, takes BLAZE_DAMAGE (all fire, armour counts) every BLAZE_TICK_MS and burns on: the player and
// the monsters alike (plan same-hazards-for-all), but a monster native to fire (Babi) is unharmed; no XP for its kill.
constexpr int BLAZE_DAMAGE = 30;
constexpr int BLAZE_TICK_MS = 500;
constexpr int BLAZE_FADE_MS = 600; // the flames die down over the last of its time
constexpr float WADE_SPEED_FACTOR_HAPI = 0.75f; // the player's wade with the Favour of Hapi skill

// The crocodile (Locomotion::Submerged): lies in the water, its top SUBMERGED_SHOW world units above the surface, until
// the player comes this close along its row. A swimmer's float height follows its target at SWIM_LIFT_RATE (1/s).
constexpr float SUBMERGED_WAKE_RANGE = 1.4f; // from its centre: its head is then ~0.65 tiles off, in view
constexpr float BOSS_WAKE_RANGE = 4.f; // a lurking boss (Sobek) wakes from farther, across its room
// A lurking charger (the hippo) wakes at the far end of its charge (ChargeRules::maxRange), so it comes out charging.
constexpr float SUBMERGED_SHOW = 0.5f; // the eyes, the nostrils and the back ridge (crocodile.py)
constexpr float SWIM_LIFT_RATE = 4.f;

// The cobra (Locomotion::Coiled) lies coiled until the player comes this close along its row, then rears up.
constexpr float COILED_WAKE_RANGE = 1.8f;
// The ushabti (Locomotion::Figurine) stands among the decor figures until the player comes this close along its row
// (from its centre), then steps off its place: next to it, so the player cannot tell it from the others before.
constexpr float FIGURINE_WAKE_RANGE = 1.3f;
// A monster's shot (MonsterShot, sim/dungeon/projectiles.h): flies straight at the player's chest as it was at the
// release, on past it until it hits a wall or the floor, or has flown SHOT_MAX_FLIGHT tiles.
constexpr float SHOT_SPEED = 3.f; // tiles per second
constexpr float SHOT_CHEST = 0.6f; // of the player's height
constexpr float SHOT_MAX_FLIGHT = 4.f;
constexpr int SHOT_SPLAT_MS = 400; // a splat on a wall or the floor stays this long
constexpr float SHOT_FAN_SPREAD = 0.3f; // radians between the shots of a fan (RangedRules::fan, Apep's spit)
// A blinding spit (plan giant-cobra-blinding-spit, RangedRules::blindMs): the view clears over the last BLIND_CLEAR_MS;
// wading, the time runs out BLIND_WATER_FACTOR times as fast (the player washes their eyes).
constexpr int BLIND_CLEAR_MS = 500;
constexpr int BLIND_WATER_FACTOR = 2;

// Burrowers (Apep, Monster::Dive): sink into the floor, stay under, come up BURROW_BEHIND tiles past the player (the
// side away from where it dived), then walk and bite for BURROW_EVERY_MS before the next dive. A hole stays drawn
// where it went down and came up for BURROW_HOLE_MS.
constexpr int BURROW_DIVE_MS = 700;
constexpr int BURROW_SURFACE_MS = 700;
constexpr int BURROW_RETRY_MS = 1000;
constexpr float BURROW_BEHIND = 1.6f;
constexpr int BURROW_HOLE_MS = 3000;
constexpr int BURROW_SPRAY_MS = 1100; // a sand burrower's spray as it comes up (plan worm-boss); Grit::BURST_MS
// Sand burrowers (the worm in sand, plan desert-chapter, Dungeon::sandTarget): lying under the sand since its spawn it
// comes up within SAND_WORM_WAKE_RANGE tiles of the player on its row; awake, it dives to come up under a player in
// its sand from SAND_WORM_DIVE_MIN to SAND_WORM_DIVE_RANGE tiles away. A lurking one stirs the sand SAND_RIPPLE_SWAY
// tiles to and fro.
constexpr float SAND_RIPPLE_SWAY = 0.12f;
// Rerek's (plan worm-boss), the worm's boss (SandBurrowRules): wakes and dives from farther, is under the sand for a
// shorter time over a longer way, dives again sooner, and every other time comes up BURROW_BEHIND past the player.
constexpr float REREK_WAKE_RANGE = 3.5f;
constexpr float REREK_DIVE_MIN = 1.1f;
constexpr float REREK_DIVE_RANGE = 9.f;
constexpr int REREK_UNDER_MS = 600;
constexpr int REREK_EVERY_MS = 4000;
// His sand spray as he comes up (RoarRules): within 1.2 tiles of him on his row, shoved 0.8 tiles away, sand over the
// screen for 0.8 s; no hit.
constexpr float REREK_SPRAY_RADIUS = 1.2f;
constexpr float REREK_SPRAY_SHOVE = 0.8f;
constexpr int REREK_SPRAY_HAZE_MS = 800;
// Held in his jaws (HoldRules::swallow) the player sinks REREK_SWALLOW_DEPTH tiles into the sand over
// REREK_SWALLOW_SINK_MS (Dungeon::PlayerSink).
constexpr float REREK_SWALLOW_DEPTH = 0.18f;
constexpr int REREK_SWALLOW_SINK_MS = 800;

// The dung ball (RollRules, plan scarab-dung-ball): after the kick the roller stands up again for
// DUNG_BALL_RECOVER_MS (the rest of its roll clip). The ball rolls at most DUNG_BALL_MAX_ROLL tiles, then falls apart;
// over a pit it falls at DUNG_BALL_FALL_GRAVITY (tiles/s^2) and breaks where it lands. Broken, its crumbs show for
// DUNG_BALL_CRUMBLE_MS. In the windup it grows from DUNG_BALL_SHAPED_FROM of its size.
constexpr int DUNG_BALL_RECOVER_MS = 250;
constexpr float DUNG_BALL_MAX_ROLL = 4.f;
constexpr float DUNG_BALL_FALL_GRAVITY = 20.f;
constexpr int DUNG_BALL_CRUMBLE_MS = 900;
constexpr float DUNG_BALL_SHAPED_FROM = 0.35f;

// Chargers (Sobek, the hippo; Monster::StartCharge): from CHARGE_MIN..CHARGE_MAX tiles along its row (Sobek's; per
// type: ChargeRules) it winds up, then rushes at CHARGE_SPEED. Its rush hits the player on the floor (not more than
// CHARGE_JUMP_CLEAR tiles up) for CHARGE_HIT_FACTOR x its damage; into a wall it is stunned and takes
// CHARGE_STUN_DAMAGE_FACTOR x the weapons' damage.
constexpr int CHARGE_WINDUP_MS = 700;
// The hippo's (plan hippo): nearer, shorter and slower than its boss-sized cousin Sobek's.
constexpr float HIPPO_CHARGE_MIN = 2.f, HIPPO_CHARGE_MAX = 4.f;
constexpr float HIPPO_CHARGE_SPEED = 3.2f;
// Seth's (plan hippo-boss), the hippo's boss: farther and faster than Sobek's.
constexpr float SETH_CHARGE_MIN = 2.5f, SETH_CHARGE_MAX = 7.f;
constexpr float SETH_CHARGE_SPEED = 4.5f;
// His storm roar as the stun ends (RoarRules): within 2 tiles of him on his row, a blunt hit, shoved 1.5 tiles away
// (at SHOVE_SPEED), sand over the screen for a second.
constexpr float SETH_ROAR_RADIUS = 2.f;
constexpr int SETH_ROAR_DAMAGE = 30;
constexpr float SETH_ROAR_SHOVE = 1.5f;
constexpr int SETH_ROAR_HAZE_MS = 1000;
// The giant rat's pack call (plan giant-rat-pack-call): once, below half its HP, every rat and giant rat within 8 tiles
// wakes and comes for the player (CallRules).
constexpr float RAT_CALL_RADIUS = 8.f;
constexpr int RAT_CALL_BELOW_PCT = 50;
constexpr float SHOVE_SPEED = 6.f; // tiles per second the player slides when shoved (Dungeon::ShovePlayer)
constexpr float CHARGE_JUMP_CLEAR = 0.15f;
constexpr int CHARGE_HIT_FACTOR = 2;
constexpr int CHARGE_STUN_MS = 2500;
constexpr int CHARGE_STUN_DAMAGE_FACTOR = 2;
constexpr int CHARGE_COOLDOWN_MS = 4000;
constexpr float CHARGE_OVERRUN = 3.f; // tiles past the player a rush with no wall ahead stops

// Holds (Monster::Grab, HoldRules; plans crocodile-hold-bite, giant-scorpion-grab): the jaws or claws open wide for
// GRAB_TELL_MS, then the lunge. It misses a player out of its reach (Monster::Reaches): backed off, jumped over it.
constexpr int GRAB_TELL_MS = 500;
// The held view (Dungeon::HeldViewRoll): the camera rolls up to HELD_ROLL_DEG with each tick of a death roll, shakes
// by HELD_SHAKE_DEG in the claws.
constexpr float HELD_ROLL_DEG = 14.f;
constexpr float HELD_SHAKE_DEG = 2.f;

// The blue lotus (Locomotion::Floating) floats as a flower until the player comes this close along its row (from its
// centre), then rears up.
constexpr float FLOATING_WAKE_RANGE = 1.2f;
// The mummy (Locomotion::Entombed) lies in its coffin (decor_coffin, drawn on its spawn tile), its body drawn
// MUMMY_COFFIN_DEPTH world units back from the walk line. Woken, its rise clip slides it out between the two fractions.
constexpr float MUMMY_WAKE_RANGE = 1.6f; // tiles along the row: in view (about 2 tiles each side)
constexpr float MUMMY_COFFIN_DEPTH = 14.4f;
constexpr float MUMMY_CLIMB_FROM = 0.3f, MUMMY_CLIMB_TO = 0.75f;

// Bats (Monster::Fly). Distances in tiles along the row, heights in world units (a tile is 40) from the floor
// to the model origin (the lowest point of the flying pose).
constexpr float BAT_SIGHT = 1.75f; // a roosting bat wakes up when the player is this close
constexpr float BAT_OVERSHOOT = 1.5f; // flies on this far past the player before it turns
constexpr int DODGE_PERCENT = 10; // the Dodge skill: a monster's attack misses this often
constexpr float BAT_BITE_REACH = 0.15f; // bites when this close in front of the player (it flies through him)
constexpr float BAT_TILES_PER_SPEED = 0.25f; // flight speed: tiles per second per speed point
constexpr float BAT_LOW_LIFT = 2.f; // height of the wing tips when passing the player (bites at head height)
constexpr float BAT_WING_DIP = 0.25f; // the wing tips reach this far below the origin (wingspans, bat.py)
constexpr float BAT_HIGH_LIFT = 19.f; // height at the turning point
constexpr float BAT_LIFT_RATE = 5.f; // how fast the height follows its target (1/s)
constexpr float BAT_CEILING = 40.f; // roosting bats hang from here
constexpr float BAT_WALL_MARGIN = 0.35f; // turns this far before a wall
constexpr int BAT_ATTACK_MS = 450; // attack clip after a bite
constexpr float BAT_FALL_GRAVITY = 600.f; // dead bats fall to the floor (world units / s^2)
constexpr int BAT_HUNT_ROWS = 3; // a hunting flyer (the boss, its minions) follows the player this many rows up or down
// A screech (plan giant-bat-screech, ScreechRules): the flyer shakes in the air through its tell, BAT_SHAKE world units
// each way. The dazed player (Player::Daze) walks at DAZE_WALK_FACTOR, cannot sprint, jump or hold a ladder; the view
// rolls up to DAZE_ROLL degrees and sways DAZE_SWAY world units, easing out over the last DAZE_CLEAR_MS.
constexpr float BAT_SHAKE = 1.2f;
constexpr float DAZE_WALK_FACTOR = 0.5f;
constexpr float DAZE_ROLL = 4.f;
constexpr float DAZE_SWAY = 2.5f;
constexpr int DAZE_CLEAR_MS = 300;

#endif
