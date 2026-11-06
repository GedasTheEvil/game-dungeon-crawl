// Walkers on the level (Monster::Seek, monster_leap.cpp, monster_climb.cpp): what blocks them, their leaps over gaps,
// a coward's flight, a climber's path across floors and the step that puts it together.
#include "dungeon.h"
#include "../entities/player.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>
#include "climb_path.h"
#include "../graphics/render_config.h"

bool Dungeon::walkerBlocked(int col, int row, bool reckless) const { return walkerBlockedAt(map, col, row, reckless); }
//======================================================================================
bool Dungeon::walkerStops(const Monster& mon, int col) const {
	if (walkerBlocked(col, mon.Row(), mon.reckless()))
		return true;
	return mon.ShunsLight() && col != static_cast<int>(std::floor(mon.CentreX())) && shunned(mon, col);
}
//======================================================================================
bool Dungeon::walkerStopsAhead(const Monster& mon, int col, int dir) const {
	return walkerStops(mon, col) || !shapePasses(map, col, mon.Row(), dir, mon.Body());
}
//======================================================================================
float Dungeon::monsterFloor(const Monster& mon) const {
	constexpr float INSET = 0.01f;
	const auto from = static_cast<int>(std::floor((mon.Left() + INSET) * SUBCELLS));
	const auto to = static_cast<int>(std::floor((mon.Right() - INSET) * SUBCELLS));
	float floor = 0.f;
	for (int k = from; k <= to; k++) {
		const int col = k >= 0 ? k / SUBCELLS : -1;
		if (!IsInBounds(col, mon.Row()))
			continue;
		const Tile& t = MapAt(col, mon.Row());
		if (hasShape(t))
			floor = std::max(floor, floorHeight(t, (static_cast<float>(k - col * SUBCELLS) + 0.5f) * SUBCELL_SIZE));
	}
	return floor;
}
//======================================================================================
// The lowest solid sub-cell of a shaped cell over its body, above its head at rest. A whole wall tile over it does
// not count (plan monster-head-bump): the halls' own ceilings are no low ceiling, or Babi (3 quarters tall) would bump
// in every one-tile room.
namespace {
constexpr float LEAP_INSET = 0.01f;
constexpr int LEAP_REACH = 3; // sub-cells above the head a leap can come to
} // namespace

float Dungeon::leapHead(const Monster& mon) const {
	return static_cast<float>(mon.Row()) + monsterFloor(mon) +
		   static_cast<float>(mon.Body().heightQuarters) * SUBCELL_SIZE;
}

std::optional<Dungeon::LeapCeiling> Dungeon::leapCeiling(const Monster& mon) const {
	const float head = leapHead(mon);
	const auto first = static_cast<int>(std::lround(head * SUBCELLS));
	for (int k = first; k < first + LEAP_REACH; k++) {
		const float y = (static_cast<float>(k) + 0.5f) * SUBCELL_SIZE;
		for (const float x : {mon.Left() + LEAP_INSET, mon.CentreX(), mon.Right() - LEAP_INSET})
			if (subcellAt(x, y))
				return LeapCeiling{(static_cast<float>(k) * SUBCELL_SIZE - head) * RenderConfig::TILE_SIZE, x, y};
	}
	return std::nullopt;
}

float Dungeon::leapRoom(const Monster& mon) const {
	const std::optional<LeapCeiling> ceiling = leapCeiling(mon);
	return ceiling ? ceiling->room : NO_CEILING;
}
//======================================================================================
int Dungeon::leapCeilingTrap(const Monster& mon) const {
	const std::optional<LeapCeiling> ceiling = leapCeiling(mon);
	if (!ceiling)
		return NoObject;
	const Tile t = Map(ceiling->x, ceiling->y);
	return spikedSubcell(t, ceiling->x - std::floor(ceiling->x), ceiling->y - std::floor(ceiling->y)) ? t.fixture.type
																									  : NoObject;
}
//======================================================================================
// Each sub-cell column the body sweeps from here to toX, up to the first solid sub-cell in a leap's reach over the
// head at the take-off.
bool Dungeon::leapUnderSpikes(const Monster& mon, float toX) const {
	const float shift = toX + static_cast<float>(mon.Col()) + 0.5f - mon.CentreX(); // Jump's toX is tile-local
	const float from = std::min(mon.Left(), mon.Left() + shift) + LEAP_INSET;
	const float to = std::max(mon.Right(), mon.Right() + shift) - LEAP_INSET;
	const auto first = static_cast<int>(std::lround(leapHead(mon) * SUBCELLS));
	const auto probes = static_cast<int>(std::ceil((to - from) / (SUBCELL_SIZE / 2.f)));
	for (int p = 0; p <= probes; p++) {
		const float at = std::min(from + static_cast<float>(p) * SUBCELL_SIZE / 2.f, to);
		for (int k = first; k < first + LEAP_REACH; k++) {
			const float y = (static_cast<float>(k) + 0.5f) * SUBCELL_SIZE;
			if (!subcellAt(at, y))
				continue;
			if (spikedSubcell(Map(at, y), at - std::floor(at), y - std::floor(y)))
				return true;
			break;
		}
	}
	return false;
}
//======================================================================================
Dungeon::StepAhead Dungeon::stepAt(const Monster& mon, float x) const {
	const auto col = static_cast<int>(std::floor(x));
	if (!IsInBounds(col, mon.Row()) || !hasShape(MapAt(col, mon.Row())))
		return StepAhead::Walk;
	const Tile& t = MapAt(col, mon.Row());
	const int column = subcellOf(x - static_cast<float>(col));
	const float h = monsterFloor(mon);
	if (!columnFits(t, column, h, mon.Body()))
		return StepAhead::Stop;
	return columnRise(t, column, h) > SUBCELL_SIZE / 2.f ? StepAhead::Hop : StepAhead::Walk;
}
//======================================================================================
Dungeon::StepAhead Dungeon::stepAhead(const Monster& mon, int dir) const {
	return stepAt(mon, mon.seekProbeX(dir) + static_cast<float>(dir) * mon.WalkStep());
}
//======================================================================================
bool Dungeon::hopOnto(Monster& mon, int dir) {
	if (dir == 0 || stepAhead(mon, dir) != StepAhead::Hop || !mon.canJump())
		return false;
	const float probe = mon.seekProbeX(dir) + static_cast<float>(dir) * mon.WalkStep();
	const float middle = (std::floor(probe * SUBCELLS) + 0.5f) * SUBCELL_SIZE; // of the step's column
	const float centre = mon.CentreX() + (middle - mon.seekProbeX(dir));
	const float toX = centre - static_cast<float>(mon.Col()) - 0.5f;
	if (shunsLeap(mon, toX)) // a coward does not hop up under ceiling spikes (plan ceiling-spikes)
		return false;
	mon.Jump(toX);
	return true;
}
//======================================================================================
bool Dungeon::shunned(const Monster& mon, int col) const {
	return mon.ShunsLight() && Lit(col, mon.Row(), mon.Type()->douseCells == 0);
}
//======================================================================================
// The nearer dark cell either way, short of a cell it can't walk on; on a tie, away from the player.
int Dungeon::wayOutOfLight(const Monster& mon) const {
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	if (!mon.ShunsLight() || !shunned(mon, col))
		return 0;
	const int away = mapX < mon.CentreX() ? 1 : -1;
	bool open[2] = {true, true}; // away, towards
	for (int d = 1; d <= 2 * LIGHT_SHUN_CELLS + 1; d++)
		for (int side = 0; side < 2; side++) {
			const int dir = side == 0 ? away : -away;
			const int c = col + dir * d;
			open[side] = open[side] && !walkerBlocked(c, mon.Row(), mon.reckless());
			if (open[side] && !shunned(mon, c))
				return dir;
		}
	return 0;
}
//======================================================================================
bool Dungeon::litBetween(const Monster& mon) const {
	const auto from = static_cast<int>(std::floor(mon.CentreX()));
	const auto to = static_cast<int>(std::floor(mapX));
	for (int col = std::min(from, to); col <= std::max(from, to); col++)
		if (col != from && shunned(mon, col))
			return true;
	return false;
}
//======================================================================================
int Dungeon::leapLanding(int col, int row, int dir, BodyFit body) const {
	return leapLandingAt(map, col, row, dir, body);
}
//======================================================================================
// The centre of the landing cell. On the player's cell: in reach of them, never past them, or the monster would turn
// and leap back over the gap.
float Dungeon::leapTarget(const Monster& mon, int land, int dir) const {
	const float centre = static_cast<float>(land) + 0.5f;
	if (static_cast<int>(std::floor(mapX)) != land)
		return centre;
	const float stop = mon.HalfWidth() + sim.player->HalfWidth() + MONSTER_BITE_REACH / 2.f;
	const float bite = mapX - static_cast<float>(dir) * stop;
	return dir > 0 ? std::clamp(bite, static_cast<float>(land), centre)
				   : std::clamp(bite, centre, static_cast<float>(land + 1));
}
//======================================================================================
bool Dungeon::pounce(Monster& mon, int dir) {
	const std::optional<PounceRules>& rules = mon.Type()->pounce;
	if (!rules || dir == 0 || !mon.canPounce() || mon.OnRungs() || !mon.sameRow(mapY))
		return false;
	const float gap = mon.PlayerGap(mapX);
	if (gap < rules->minGap || gap > rules->maxGap)
		return false;
	const auto to = static_cast<int>(std::floor(mapX));
	const auto front = static_cast<int>(std::floor(mon.seekProbeX(dir)));
	for (int col = front; (to - col) * dir >= 0; col += dir)
		if (col == front ? walkerStops(mon, col) || stepAhead(mon, dir) != StepAhead::Walk
						 : walkerStopsAhead(mon, col, dir))
			return false;
	// It bumped its head lately and is still under a low ceiling (plan monster-head-bump): no second bump at once.
	if (mon.BumpedLately() && leapRoom(mon) < MONSTER_JUMP_HEIGHT)
		return false;
	const float stop = mon.HalfWidth() + sim.player->HalfWidth() + rules->landGap;
	mon.Pounce(mapX - static_cast<float>(dir) * stop - static_cast<float>(mon.Col()) - 0.5f, dir);
	return true;
}
//======================================================================================
bool Dungeon::walkerReaches(const Monster& mon, int dir) const {
	const auto to = static_cast<int>(std::floor(mapX));
	const bool leaps = ::leaps(mon.Type()->locomotion);
	const auto front = static_cast<int>(std::floor(mon.seekProbeX(dir)));
	for (int col = front; (to - col) * dir > 0; col += dir) {
		// A step or an opening it does not pass ends the way; a leap is over a gap only.
		if (col == front ? stepAhead(mon, dir) == StepAhead::Stop : !shapePasses(map, col, mon.Row(), dir, mon.Body()))
			return false;
		if (!walkerStops(mon, col))
			continue;
		const int land = leaps ? leapLanding(col, mon.Row(), dir, mon.Body()) : -1;
		if (land < 0)
			return false;
		col = land;
	}
	return !mon.ShunsLight() || !shunned(mon, to); // nor into the light around them
}
//======================================================================================
// No way off the row for a walker yet, so out of the player's fire means behind a wall or far enough along the row.
void Dungeon::fleeFrom(Monster& mon, int dir) {
	const float gap = dir > 0 ? (mapX - sim.player->HalfWidth()) - mon.Right()
							  : mon.Left() - (mapX + sim.player->HalfWidth());
	if (gap >= COWARD_SAFE_GAP || !clearRow(mon.CentreX(), mapX, mon.Row())) {
		mon.Stand();
		return;
	}
	const auto col = static_cast<int>(std::floor(mon.seekProbeX(-dir)));
	mon.Flee(walkerStops(mon, col) || stepAhead(mon, -dir) != StepAhead::Walk, -dir);
}
//======================================================================================
// Threatened: the player nearer than near and coming at it. Once it backs off, it goes on while they keep coming, so
// it has the time for a shot at far, else KITE_HYSTERESIS past near. Never past far, and never off its floor: cornered
// (no kiteRoom for a step), it stays.
bool Dungeon::kiteThreatened(const Monster& mon, int dir) const {
	const std::optional<KiteRules>& kite = mon.Type()->kite;
	if (!kite || dir == 0)
		return false;
	const float gap = mon.PlayerGap(mapX);
	const bool backing = mon.FleeDir() == -dir;
	const bool closing = mon.PlayerClosing();
	const float room = MONSTER_SEEK_STEP * kite->backOffSpeed; // one step back
	return gap + room <= kite->far &&
		   (backing ? closing || gap < kite->near + KITE_HYSTERESIS : gap < kite->near && closing);
}
//======================================================================================
int Dungeon::kiteAway(const Monster& mon, int dir) const {
	const std::optional<KiteRules>& kite = mon.Type()->kite;
	if (!kite || !kiteThreatened(mon, dir))
		return 0;
	const float room = MONSTER_SEEK_STEP * kite->backOffSpeed; // one step back
	return kiteRoom(mon, dir, room) < room ? 0 : -dir; // cornered: no step back
}
//======================================================================================
// Its whole body on the far side of the gap, its front at the gap's edge where the ledge is long enough.
std::optional<float> Dungeon::leapAwayTarget(const Monster& mon, int dir, bool threatened) const {
	const std::optional<KiteRules>& kite = mon.Type()->kite;
	if (!kite || !leaps(mon.Type()->locomotion) || !mon.canJump() || kiteAway(mon, dir) != 0 ||
		(threatened && !kiteThreatened(mon, dir)))
		return std::nullopt;
	constexpr float PROBE = 0.05f;
	const float step = MONSTER_SEEK_STEP * kite->backOffSpeed; // the step back it has no room for
	const auto behind = static_cast<int>(std::floor(mon.seekProbeX(-dir) - static_cast<float>(dir) * (step + PROBE)));
	if (!walkerBlocked(behind, mon.Row(), mon.reckless()) || isSolidTile(MapAt(behind, mon.Row())))
		return std::nullopt; // cornered by a wall or another monster: it fights
	const int land = leapLanding(behind, mon.Row(), -dir, mon.Body());
	if (land < 0)
		return std::nullopt;
	const float edge = static_cast<float>(dir < 0 ? land : land + 1); // where the ledge begins
	const float to = edge - static_cast<float>(dir) * (mon.HalfWidth() + PROBE);
	// Not threatened, only nearer than its band: only to a place it can throw from, or it would come back at once.
	if (!threatened && std::fabs(to - mapX) - mon.HalfWidth() - sim.player->HalfWidth() > kite->far)
		return std::nullopt;
	return to;
}
//======================================================================================
bool Dungeon::leapAway(Monster& mon, int dir, bool threatened) {
	const std::optional<float> to = leapAwayTarget(mon, dir, threatened);
	if (!to || shunsLeap(mon, *to - static_cast<float>(mon.Col()) - 0.5f))
		return false;
	mon.Jump(*to - static_cast<float>(mon.Col()) - 0.5f);
	sim.journal->SeeMove(mon.Type()->id, levelNumber, CreatureMove::LeapAway);
	return true;
}
//======================================================================================
// The free run behind it, away from the player (dir), up to upTo tiles: to a wall, a drop, a trap (also for a reckless
// one), a lit cell for a light shunner, or another monster on its row (not a boss's minion for the boss, nor the boss
// for its minion).
float Dungeon::kiteRoom(const Monster& mon, int dir, float upTo) const {
	const float rear = mon.seekProbeX(-dir);
	float room = upTo;
	for (const Monster& other : monsters) {
		if (&other == &mon || !other.Active() || !other.Alive() || other.flies() || other.Row() != mon.Row() ||
			(other.CentreX() - mon.CentreX()) * static_cast<float>(dir) > 0.f)
			continue;
		if ((mon.Type()->isBoss() && other.Minion()) || (other.Type()->isBoss() && mon.Minion()))
			continue; // a boss and its minions make way for each other (Babi's baboons, plan babi-boss)
		room = std::min(room, std::max(0.f, dir > 0 ? mon.Left() - other.Right() : other.Left() - mon.Right()));
	}
	constexpr float PROBE = 0.05f;
	const auto probes = static_cast<int>(room / PROBE);
	for (int i = 1; i <= probes; i++) {
		const auto col = static_cast<int>(std::floor(rear - static_cast<float>(dir * i) * PROBE));
		if (walkerBlocked(col, mon.Row(), false) || (mon.ShunsLight() && shunned(mon, col)) ||
			stepAt(mon, rear - static_cast<float>(dir * i) * PROBE) != StepAhead::Walk)
			return static_cast<float>(i - 1) * PROBE;
	}
	return room;
}
//======================================================================================
//======================================================================================
// In its band it stands and shoots. Cornered nearer than near (no room behind to get back to it), it fights: the
// walker's seek and bite; a leaper with a gap behind it leaps that first.
bool Dungeon::kite(Monster& mon, int dir) {
	const std::optional<KiteRules>& kite = mon.Type()->kite;
	if (!kite || dir == 0 || !walkerReaches(mon, dir))
		return false;
	if (const int away = kiteAway(mon, dir); away != 0) {
		mon.BackOff(away);
		return true;
	}
	if (leapAway(mon, dir)) // a leaper cornered at a gap: over it to the next ledge (plan babi-boss)
		return true;
	const float gap = mon.PlayerGap(mapX);
	if (gap > kite->far)
		return false;
	if (gap + kiteRoom(mon, dir, kite->near) <
		kite->near) // cornered nearer than its band: a leaper leaps, else it bites
		return leapAway(mon, dir, false);
	mon.Stand();
	return true;
}
//======================================================================================
// The ladder the climber heads for: the run of rungs along the path from (col, row), up or down, to where the path
// leaves the ladder.
int Dungeon::climbEnd(int col, int row) const {
	int r = row, nc = 0, nr = 0;
	while (climbPath.Next(col, r, nc, nr) && nc == col && nr != r)
		r = nr;
	return r;
}
//======================================================================================
// No bite from the rungs (plan climber-off-the-rungs): the side it steps off to, where it stands on a floor; the
// player's side first, so standing next to a ladder is no safe spot. 0: not on the rungs, or no floor either side.
int Dungeon::rungsExit(const Monster& mon) const {
	if (!mon.OnRungs())
		return 0;
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	const int towards = mapX < mon.CentreX() ? -1 : 1;
	for (const int side : {towards, -towards})
		if (!walkerBlocked(col + side, mon.Row(), mon.reckless()))
			return side;
	return 0;
}
//======================================================================================
// A climber with no way on that hangs on the rungs over the shaft (it came up after the player, who then went on
// into the light, plan spider-climbers) climbs down to the first floor below. False: it stands on a floor.
bool Dungeon::climbDown(Monster& mon) {
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	int row = mon.Row();
	while (mon.OnRungs() && row > 0 && walkerBlocked(col, row, mon.reckless()) && MapAt(col, row - 1).type == Ladder)
		row--;
	if (row == mon.Row())
		return false;
	mon.StartClimb(col, row);
	return true;
}
//======================================================================================
bool Dungeon::followPath(Monster& mon, int dir) {
	if (!mon.climbs() || !mon.Alerted())
		return false;
	if (mon.Climbing()) { // a climb once started runs to its end
		mon.UpdateClimb();
		return true;
	}
	const auto playerCol = static_cast<int>(std::floor(mapX));
	const auto playerRow = static_cast<int>(std::floor(mapY + STANDING_EPSILON));
	if (mon.sameRow(mapY) && (dir == 0 || walkerReaches(mon, dir))) {
		mon.SawWay(playerCol, playerRow);
		if (const int off = rungsExit(mon); off != 0) {
			mon.WalkPath(off, false);
			return true;
		}
		return false; // the walker's rules: chase along the row and bite
	}
	ClimbAbility how{
		mon.reckless(),
		leaps(mon.Type()->locomotion),
		mon.Body(),
		MONSTER_JUMP_MAX_GAP + (mon.Type()->isBoss() ? BOSS_LEAP_CATCH_EXTRA : 0)
	};
	if (mon.ShunsLight()) { // no way through the light, a lit ladder top is a wall (plan spider-climbers)
		const bool torches = mon.Type()->douseCells == 0; // as shunned
		how.avoids = [this, torches](int c, int r) { return Lit(c, r, torches); };
	}
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	climbPath.Build(map, playerCol, playerRow, how, CLIMB_PATH_MAX);
	if (climbPath.Steps(col, mon.Row()) > 0)
		mon.SawWay(playerCol, playerRow);
	else if (int toCol = 0, toRow = 0; mon.HeadingFor(toCol, toRow))
		climbPath.Build(map, toCol, toRow, how, CLIMB_PATH_MAX); // where the player was last
	else
		return climbDown(mon); // gave up: it stays where it is, on a floor
	int nextCol = 0, nextRow = 0;
	if (!climbPath.Next(col, mon.Row(), nextCol, nextRow))
		return climbDown(mon);
	if (nextRow != mon.Row()) { // a rung: to the ladder's middle first
		const float centre = static_cast<float>(col) + 0.5f;
		if (std::fabs(mon.CentreX() - centre) > mon.WalkStep())
			mon.WalkPath(centre > mon.CentreX() ? 1 : -1, false);
		else
			mon.StartClimb(col, climbEnd(col, mon.Row()));
		return true;
	}
	const int step = nextCol > col ? 1 : -1;
	if (std::abs(nextCol - col) > 1) { // a leap over a gap
		const int rungsTo = MapAt(nextCol, mon.Row()).type == Ladder ? climbEnd(nextCol, mon.Row()) : mon.Row();
		if (!mon.canJump() || shunsLeap(mon, static_cast<float>(nextCol - mon.Col())))
			mon.WalkPath(step, true);
		else if (rungsTo != mon.Row()) // the way climbs on from there: the paws catch the rungs in the air
			mon.LeapToRungs(nextCol, rungsTo);
		else
			mon.Jump(static_cast<float>(nextCol - mon.Col()));
		return true;
	}
	if (hopOnto(mon, step))
		return true;
	const auto probe = static_cast<int>(std::floor(mon.seekProbeX(step)));
	mon.WalkPath(
		step,
		(probe != col && probe != nextCol && !climbPath.Stands(probe, mon.Row())) ||
			stepAhead(mon, step) != StepAhead::Walk
	);
	return true;
}
//======================================================================================
// One step of a walker that is due: a climber's path, a kiter's back-off, a coward's flight, a leap over a gap, the
// chase and the bite.
void Dungeon::updateWalker(Monster& mon) {
	if (!mon.Alive() || monstersStandDown() || !mon.StepDue())
		return;
	int dir = mon.attackDirection(mapX, mapY);
	if (followPath(mon, dir))
		return;
	if (const int out = wayOutOfLight(mon); out != 0) { // a light shunner caught in the light runs for the dark
		mon.Flee(false, out);
		return;
	}
	if (kite(mon, dir))
		return;
	// A coward, or any walker the player hit (plan hit-wakes-monster; a boss stands its ground), runs from a player it
	// cannot reach: along its row, or off another row it has no way to.
	const bool runs = mon.flees() || (mon.Provoked() && !mon.rooted() && !mon.Type()->isBoss());
	if (dir == 0 && runs && mon.Provoked() && !mon.sameRow(mapY) &&
		static_cast<int>(std::floor(mapY + STANDING_EPSILON)) != mon.Row()) {
		fleeFrom(mon, mapX < mon.CentreX() ? -1 : 1);
		return;
	}
	if (dir != 0 && runs && !walkerReaches(mon, dir)) {
		if (mon.ShunsLight() && mon.sameRow(mapY) && litBetween(mon))
			sim.journal->SeeMove(mon.Type()->id, levelNumber, CreatureMove::ShunLight);
		fleeFrom(mon, dir);
		return;
	}
	if (pounce(mon, dir))
		return;
	auto col = static_cast<int>(std::floor(mon.seekProbeX(dir)));
	const bool cellBlocked = walkerStops(mon, col);
	if (!cellBlocked && hopOnto(mon, dir)) // a leaper onto a step (plan subcell-monsters)
		return;
	const bool blocked = cellBlocked || (dir != 0 && stepAhead(mon, dir) != StepAhead::Walk);
	if (cellBlocked && dir != 0 && mon.canJump()) { // a leap over a gap
		int land = leapLanding(col, mon.Row(), dir, mon.Body());
		// Not while the player is in the gap: the rat would leap over them.
		float gapFrom = static_cast<float>(dir > 0 ? col : land + 1);
		float gapTo = static_cast<float>(dir > 0 ? land : col + 1);
		const float toX = land >= 0 ? leapTarget(mon, land, dir) - static_cast<float>(mon.Col()) - 0.5f : 0.f;
		if (land >= 0 && (mapX < gapFrom || mapX >= gapTo) && !shunsLeap(mon, toX)) {
			mon.Jump(toX);
			return;
		}
	}
	if (!mon.Seek(blocked, mapX, mapY))
		if (mon.AttackDue() && !mon.OnRungs())
			mon.Attack(mapX, mapY, PlayerWading());
}
