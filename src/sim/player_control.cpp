#include "player_control.h"
#include "dungeon/dungeon.h"
#include "player.h"
#include "../core/timer.h"
#include "../level/tuning/gameplay_shared.h"
#include "../level/rules/items.h"

BagUser bagUser(const Player& player) {
	const PlayerStats& s = player.stats;
	return {
		player.Alive(),
		s.CurrentHP(),
		s.CurrentMaxHP(),
		s.Stamina(),
		s.MaxStamina(),
		s.vitals.poison.Any(),
		s.PotionResistPercent()
	};
}

PlayerControl::Began PlayerControl::Press(int facing) {
	if (player.CanCharge(isRanged(bag.Equipped())) && player.attackStartMs < 0 && player.attackTimer.TimePassed(true)) {
		if (!attackReady())
			return Began::None; // refused (a ladder, a hold), told: nothing more
		if (player.StartCharge())
			return Began::Charge;
		// too little stamina for a charge (the HUD sign blinks): the plain attack
	}
	return Attack(facing);
}

PlayerControl::Began PlayerControl::Attack(int facing) {
	if (!attackReady())
		return Began::None;
	const ItemKind weapon = bag.Equipped();
	if (player.stats.curses.Has(Curse::Palsy) && isRanged(weapon)) {
		if (!refusalTold)
			events.Status("Your hands shake too much to aim");
		refusalTold = true; // no shot, no ammo, no cooldown
		return Began::None;
	}
	// No shot left, or a monster too close (plan ranged-weaker-than-melee): no draw, nothing spent.
	if (const Dungeon::ShotRefusal refused = dungeon.ShotRefused(weapon, facing);
		refused != Dungeon::ShotRefusal::None) {
		dungeon.RefuseShot(refused);
		return Began::None;
	}
	player.swingPower = 0;
	swing(false);
	return Began::Swing;
}

PlayerControl::Began PlayerControl::ReleaseCharge() {
	const bool charged = player.ReleaseCharge() > 0;
	if (!player.Alive() || !attackReady())
		return Began::None;
	swing(charged);
	return charged ? Began::ChargedSwing : Began::Swing;
}

bool PlayerControl::UpdateAttack(int facing, float handHeight) {
	if (player.attackStartMs < 0)
		return false;
	const ItemKind kind = bag.Equipped();
	const WeaponDef& def = weaponDef(kind);
	const int t = GameClock::now() - player.attackStartMs;
	bool struck = false;
	if (!player.attackLanded && t >= def.motion.hitMs) {
		player.attackLanded = true;
		const int damage = player.stats.Damage(weaponDamage(kind, def.damage, bag.Level(kind)));
		const float reach = player.stats.WeaponReach(kind);
		if (isRanged(kind)) {
			// A monster come too close during the draw stops the shot (plan ranged-weaker-than-melee): the swing ends.
			if (!dungeon.ShootWeapon(kind, damage, facing, handHeight, reach)) {
				player.attackStartMs = -1;
				return false;
			}
			struck = true;
		} else // with a power hit's bonus (plan skill-power-hit)
			struck = dungeon.AttackNearest(
				player.PowerDamage(damage),
				def.mix,
				reach,
				facing,
				player.SwingShare(),
				def.testOnly
			);
	}
	if (t >= def.motion.swingMs)
		player.attackStartMs = -1;
	return struck;
}

float PlayerControl::MoveMultiplier() const {
	return dungeon.PlayerSprintMultiplier(player.stats.SprintMoveMultiplier());
}

bool PlayerControl::Walk(float dx, float dy) {
	const bool sprints = dungeon.PlayerCanSprint();
	const float factor = dungeon.PlayerStepFactor();
	const bool moved = dungeon.Move(dx * factor, dy * factor);
	if (moved && sprints)
		player.stats.NoteWalked(dungeon.PlayerHalvedWadeDrain());
	return moved;
}

void PlayerControl::AfterUpdate() {
	player.stats.UpdateStamina(events);
	if (player.infiniteStamina)
		player.stats.SetStamina(player.stats.MaxStamina());
	player.UpdatePoison(events);
	player.UpdateBleeding(dungeon.PlayerWading(), events);
	player.stats.Regenerate(UPDATE_TICK_MS);
	dungeon.AnimateMonsters();
	player.Animate();
}

bool PlayerControl::attackReady() {
	if (player.attackStartMs >= 0 || !player.attackTimer.TimePassed(true))
		return false;
	if (!dungeon.AttackAllowed(!refusalTold)) {
		refusalTold = true;
		return false;
	}
	return true;
}

void PlayerControl::swing(bool charged) {
	const WeaponMotion& motion = weaponDef(bag.Equipped()).motion;
	player.attackTimer.Reset();
	player.attackTimer.SetInterval(motion.AttackMs());
	player.attackStartMs = GameClock::now() - (charged ? windupMs(motion) : 0);
	player.attackLanded = false;
}
