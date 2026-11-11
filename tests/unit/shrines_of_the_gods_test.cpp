// The shrines of Montu, Ptah, Amun and Serket (plan shrines-of-the-gods): a prayer gives the god's blessing for 2
// minutes (Serket's 4 with Serket's favour). Montu Might +5, Ptah Armor +5, Amun a full heal and max HP +20%, Serket
// cures poison and no poison lands. They end on death, the same god restarts the time, different gods stack.
#include "../../external/doctest/doctest.h"
#include "../../src/level/decor.h"
#include "../../src/level/shrines.h"
#include "sim_world.h"
#include "../../src/level/journal_text.h"
#include <algorithm>
#include <string>

namespace {
constexpr int ROW = 2; // tests/levels/interact: the corridor's row
constexpr int SHRINE_COL = 4; // a cell with nothing to interact with
constexpr God NEW_GODS[] = {God::Montu, God::Ptah, God::Amun, God::Serket};

bool said(const std::vector<WorldEvent>& events, const std::string& text) {
	return std::any_of(events.begin(), events.end(), [&](const WorldEvent& e) {
		return e.text.find(text) != std::string::npos;
	});
}
} // namespace

TEST_CASE("Shrines of the gods: each shrine's prop prays to its god, with its sound and field note") {
	for (const God g : NEW_GODS) {
		CAPTURE(godDef(g).name);
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/interact"));
		world.WaitTicks(30);
		REQUIRE(world.dungeon.PlaceDecor(SHRINE_COL, ROW, godDef(g).decor));
		REQUIRE(world.WalkTo(SHRINE_COL + 0.5f));
		CHECK(world.dungeon.InteractVerb() == Interaction::Pray);
		world.Interact();
		world.WaitTicks(1);
		CHECK(world.player.stats.Blessed(g));
		CHECK_FALSE(world.player.stats.IsisWarded());
		CHECK(world.player.stats.PrayedGod() == g); // the beam in the god's tint
		const auto sound = static_cast<WorldSound>(static_cast<int>(WorldSound::ShrinePray) + static_cast<int>(g));
		CHECK(std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
			return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
		}));
		CHECK(std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
			return e.kind == WorldEvent::Kind::Note && e.note == shrineNote(g);
		}));
		CHECK(world.dungeon.InteractVerb() == Interaction::Pray); // no cooldown
	}
}

TEST_CASE("Shrines of the gods: Montu Might +5, Ptah Armor +5, both at once") {
	Player player;
	WorldEvents events;
	const int might = player.stats.CurrentMight(), armor = player.stats.CurrentArmor();
	player.stats.Pray(God::Montu, events);
	CHECK(player.stats.CurrentMight() == might + MONTU_MIGHT);
	CHECK(player.stats.CurrentArmor() == armor);
	player.stats.Pray(God::Ptah, events);
	CHECK(player.stats.CurrentMight() == might + MONTU_MIGHT); // the gods stack
	CHECK(player.stats.CurrentArmor() == armor + PTAH_ARMOR);
	CHECK(MONTU_MIGHT == 5);
	CHECK(PTAH_ARMOR == 5);
}

TEST_CASE("Shrines of the gods: a blessing lasts 2 minutes, a prayer at the same god restarts it") {
	Player player;
	WorldEvents events;
	const int might = player.stats.CurrentMight();
	player.stats.Pray(God::Montu, events);
	CHECK(player.stats.BlessingLeftMs(God::Montu) == BLESSING_MS);
	CHECK(BLESSING_MS == 120000);
	player.stats.AdvanceBlessings(90000, events);
	CHECK(player.stats.BlessingLeftMs(God::Montu) == 30000);
	player.stats.Pray(God::Montu, events); // again: the full time, not more Might
	CHECK(player.stats.BlessingLeftMs(God::Montu) == BLESSING_MS);
	CHECK(player.stats.CurrentMight() == might + MONTU_MIGHT);
	static_cast<void>(events.Take());
	player.stats.AdvanceBlessings(BLESSING_MS, events);
	CHECK_FALSE(player.stats.Blessed(God::Montu));
	CHECK(player.stats.CurrentMight() == might);
	CHECK(said(events.Take(), "The Fury of Montu fades"));
}

TEST_CASE("Shrines of the gods: Amun heals fully and raises max HP 20%; at the end the HP stays within the max") {
	Player player;
	WorldEvents events;
	const int max = player.stats.CurrentMaxHP();
	player.stats.LoseHP(max / 2);
	player.stats.Pray(God::Amun, events);
	CHECK(player.stats.CurrentMaxHP() == max + max * AMUN_MAX_HP_PERCENT / 100);
	CHECK(player.stats.CurrentHP() == player.stats.CurrentMaxHP());
	player.stats.AdvanceBlessings(BLESSING_MS, events);
	CHECK(player.stats.CurrentMaxHP() == max);
	CHECK(player.stats.CurrentHP() == max); // clamped to the normal max

	player.stats.Pray(God::Amun, events);
	player.stats.LoseHP(player.stats.CurrentHP() - 3); // hurt below the normal max: kept
	player.stats.AdvanceBlessings(BLESSING_MS, events);
	CHECK(player.stats.CurrentHP() == 3);
}

TEST_CASE("Shrines of the gods: Serket cures poison and wards it off; twice as long with Serket's favour") {
	Player player;
	WorldEvents events;
	Rng rng(1);
	player.Poison(PoisonTier::Strong, events, rng);
	REQUIRE(player.stats.vitals.poison.Any());
	player.stats.Pray(God::Serket, events);
	CHECK_FALSE(player.stats.vitals.poison.Any());
	CHECK(player.stats.BlessingLeftMs(God::Serket) == BLESSING_MS);
	static_cast<void>(events.Take());
	for (int t = 0; t < POISON_TIER_COUNT; t++)
		player.Poison(static_cast<PoisonTier>(t), events, rng);
	CHECK_FALSE(player.stats.vitals.poison.Any());
	CHECK(said(events.Take(), "Serket turns the venom away"));
	player.stats.AdvanceBlessings(BLESSING_MS, events);
	player.Poison(PoisonTier::Weak, events, rng);
	CHECK(player.stats.vitals.poison.Any()); // over: poison lands again

	Player favoured;
	favoured.stats.heka.Earn(5);
	REQUIRE(favoured.stats.heka.Learn(SkillId::SerketsFavour, true));
	favoured.stats.Pray(God::Serket, events);
	CHECK(favoured.stats.BlessingLeftMs(God::Serket) == 2 * BLESSING_MS);
}

TEST_CASE("Shrines of the gods: every blessing ends on death and on a new game, kept over a save and load") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/interact"));
	for (int g = 0; g < GOD_COUNT; g++)
		world.player.stats.Pray(static_cast<God>(g), world.events);
	world.Wait(1000);
	const int left = world.player.stats.BlessingLeftMs(God::Ptah);
	REQUIRE(left > 0);
	world.SaveAndLoad();
	for (int g = 0; g < GOD_COUNT; g++)
		CHECK(world.player.stats.BlessingLeftMs(static_cast<God>(g)) == left);
	world.player.TakeHit(100000, DamageMix{100, 0, 0}, world.events, true);
	REQUIRE_FALSE(world.player.Alive());
	for (int g = 0; g < GOD_COUNT; g++)
		CHECK_FALSE(world.player.stats.Blessed(static_cast<God>(g)));
	world.player.Reanimate(); // a new game
	world.player.stats.Pray(God::Amun, world.events);
	world.player.Reanimate();
	CHECK_FALSE(world.player.stats.Blessed(God::Amun));
	CHECK(world.player.stats.CurrentHP() == world.player.stats.CurrentMaxHP());
}

TEST_CASE("Shrines of the gods: the props, the glyphs, the notes saved by number with a text") {
	for (int g = 0; g < GOD_COUNT; g++)
		CHECK(shrineGod(GODS[g].decor) == static_cast<God>(g));
	CHECK_FALSE(shrineGod(DECOR_STARGATE));
	CHECK(std::string(DECOR_NAMES[DECOR_SHRINE_SERKET]) == "shrine_serket");
	CHECK(decorGlyph(DECOR_SHRINE_ISIS) == '|');
	CHECK(decorGlyph(DECOR_SHRINE_MONTU) == 'M');
	CHECK(decorGlyph(DECOR_SHRINE_SERKET) == 'S');
	CHECK(static_cast<int>(FieldNote::ShrineOfMontu) == static_cast<int>(FieldNote::QuiverOfNeith) + 1);
	CHECK(static_cast<int>(FieldNote::SkyIron) == static_cast<int>(FieldNote::ShrineOfSerket) + 1);
	CHECK(static_cast<int>(FieldNote::SkyIron) + 1 == FIELD_NOTE_COUNT); // plan sky-iron-rings
	for (const God g : NEW_GODS) {
		CAPTURE(godDef(g).name);
		CHECK(std::string(fieldNoteText(shrineNote(g)).title) == std::string("The Shrine of ") + godDef(g).name);
		CHECK(std::string(fieldNoteText(shrineNote(g)).text) != "");
	}
}

TEST_CASE(
	"Learning a skill (plan learn-skill-cures): Serket's favour cures the poison, Blessing of Isis lifts the curses"
) {
	Player player;
	WorldEvents events;
	Rng rng(1);
	for (int boss = 1; boss <= 3; boss++)
		player.stats.heka.Earn(boss * 5);
	player.Poison(PoisonTier::Strong, events, rng);
	player.stats.curses.Lay(Curse::Weakness, CurseGrade::Greater);
	player.stats.curses.Lay(Curse::Slowness, CurseGrade::Lesser);
	LearnCure cure = LearnCure::None;
	REQUIRE(player.stats.Learn(SkillId::Dodge, true, &cure)); // another skill: nothing cured
	CHECK(cure == LearnCure::None);
	CHECK(player.stats.vitals.poison.Any());
	CHECK(player.stats.curses.Any());
	REQUIRE(player.stats.Learn(SkillId::SerketsFavour, true, &cure));
	CHECK(cure == LearnCure::Poison);
	CHECK_FALSE(player.stats.vitals.poison.Any());
	CHECK(player.stats.curses.Any()); // only the poison
	CHECK_FALSE(player.stats.Blessed(God::Serket)); // no ward
	REQUIRE(player.stats.Learn(SkillId::BlessingOfIsis, true, &cure));
	CHECK(cure == LearnCure::Curses);
	CHECK_FALSE(player.stats.curses.Any());
	CHECK_FALSE(player.stats.IsisWarded());

	Player healthy; // nothing running: nothing to say
	healthy.stats.heka.Earn(5);
	healthy.stats.heka.Earn(10);
	REQUIRE(healthy.stats.Learn(SkillId::SerketsFavour, true, &cure));
	CHECK(cure == LearnCure::None);
	REQUIRE(healthy.stats.Learn(SkillId::BlessingOfIsis, true, &cure));
	CHECK(cure == LearnCure::None);
}
