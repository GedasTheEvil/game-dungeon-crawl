// The rings (plan ring-accessories): two worn at once, one per hand; the gem's effect, the metal's strength, the sums
// and caps, the hands, the save, the loot.
#include "../../external/doctest/doctest.h"
#include "../../src/level/player_stats.h"
#include "../../src/level/item_bag.h"
#include "../../src/level/journal.h"
#include "../../src/level/journal_text.h"
#include "../../src/level/loot.h"
#include "../../src/level/movement.h"
#include "sim_world.h"
#include <sstream>
#include <string>

namespace {
constexpr BagUser ALIVE{true, 1, 1, 1, 1};

WornRings worn(std::optional<ItemKind> left, std::optional<ItemKind> right) { return {left, right}; }

ItemBag bagWith(std::initializer_list<ItemKind> rings) {
	ItemBag bag;
	for (ItemKind ring : rings)
		bag.Add(ring);
	return bag;
}

// The player's x after walking right for ms on tests/levels/empty_row with the rings on; jump: one jump first, the
// walk key held through it.
float walked(std::optional<ItemKind> ring, int ms, bool jump) {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	if (ring)
		world.WearRing(*ring, Hand::Left);
	const float from = world.X();
	world.facing = 1;
	if (jump) {
		world.Jump();
		while (world.player.jump.jumping || world.player.jump.falling) {
			world.Step(1);
			world.Tick();
		}
		return world.X() - from;
	}
	world.HoldWalk(1, ms);
	return world.X() - from;
}
} // namespace

TEST_CASE("rings: six gems, four metals, named by both") {
	CHECK(isRing(ItemKind::GarnetCopper));
	CHECK_FALSE(isAmulet(ItemKind::GarnetCopper));
	CHECK_FALSE(isRing(ItemKind::PiercingGrand));
	CHECK(itemGroup(ItemKind::AmethystGold) == ItemGroup::Rings);
	for (int i = FIRST_RING; i < ITEM_KIND_COUNT; i++) {
		const Ring r = ringOf(itemAt(i));
		CHECK(ringKind(r.gem, r.metal) == itemAt(i));
		const ItemFileId id = fileIdOf(itemAt(i));
		CHECK(id.type == ItemType::RING);
		CHECK(itemFromFile(id.type, id.id) == itemAt(i));
	}
	CHECK_FALSE(itemFromFile(ItemType::RING, RING_KIND_COUNT).has_value());
	// The file ids of the rings from before sky iron keep reading (plan sky-iron-rings); sky iron comes after them.
	CHECK(itemFromFile(ItemType::RING, 17) == ItemKind::AmethystGold);
	CHECK(itemFromFile(ItemType::RING, 3) == ItemKind::LapisCopper);
	CHECK(fileIdOf(ItemKind::GarnetSkyIron).id == ItemType::SKY_IRON_RING_FILE_ID);
	CHECK(itemFromFile(ItemType::RING, ItemType::SKY_IRON_RING_FILE_ID + 5) == ItemKind::AmethystSkyIron);
	CHECK(std::string(itemText(ItemKind::GarnetSkyIron).name) == "Sky-Iron Garnet Ring");
	CHECK(std::string(itemText(ItemKind::GarnetSkyIron).shortName) == "Garnet IV");
	CHECK(std::string(itemText(ItemKind::GarnetSkyIron).label) == "sky-iron ring of regeneration");
	CHECK(std::string(itemText(ItemKind::GarnetSkyIron).effect) == "Heals 1% health (at least 1) every 2 s");
	CHECK(std::string(itemText(ItemKind::LapisSkyIron).lore1) == "Iron of heaven,");
	CHECK(std::string(itemText(ItemKind::JasperSkyIron).effect) == "Walk and sprint 17% faster");
	CHECK(std::string(ringBandTexture(RingMetal::SkyIron)) == "ring_band_sky_iron");
	CHECK(std::string(itemText(ItemKind::GarnetGold).name) == "Gold Garnet Ring");
	CHECK(std::string(itemText(ItemKind::GarnetGold).shortName) == "Garnet III");
	CHECK(std::string(itemText(ItemKind::GarnetGold).label) == "gold ring of regeneration");
	CHECK(std::string(itemText(ItemKind::GarnetGold).effect) == "Heals 1 health every 3 s");
	CHECK(std::string(itemText(ItemKind::AmethystCopper).shortName) == "Amethyst I");
	CHECK(std::string(itemText(ItemKind::JasperSilver).effect) == "Walk and sprint 8% faster");
	CHECK(std::string(itemText(ItemKind::CrystalGold).lore1) != "");
}

TEST_CASE("the ring bonus: both hands count, the same gem stacks, two identical rings too, up to the caps") {
	CHECK(ringBonus(worn(std::nullopt, std::nullopt)).regenHpPerHour == 0);
	const RingBonus mixed = ringBonus(worn(ItemKind::LapisSilver, ItemKind::FeldsparGold));
	CHECK(mixed.staminaRegenPercent == 25);
	CHECK(mixed.poisonFasterPercent == 50);
	CHECK(mixed.speedPercent == 0);
	// Two gold rubies: 1 HP every 1.5 s.
	CHECK(
		ringBonus(worn(ItemKind::GarnetGold, ItemKind::GarnetGold)).regenHpPerHour == 2 * RingEffect::MS_PER_HOUR / 3000
	);
	CHECK(ringBonus(worn(ItemKind::JasperCopper, ItemKind::JasperCopper)).speedPercent == 10);
	CHECK(ringBonus(worn(ItemKind::JasperCopper, ItemKind::JasperSilver)).speedPercent == 13);
	CHECK(ringBonus(worn(ItemKind::JasperGold, ItemKind::JasperGold)).speedPercent == 24);
	CHECK(ringBonus(worn(ItemKind::JasperSkyIron, ItemKind::JasperGold)).speedPercent == RingEffect::JASPER_CAP);
	CHECK(ringBonus(worn(ItemKind::CrystalGold, ItemKind::CrystalGold)).reachPercent == 50);
	CHECK(ringBonus(worn(ItemKind::CrystalSkyIron, ItemKind::CrystalSkyIron)).reachPercent == RingEffect::CRYSTAL_CAP);
	CHECK(ringBonus(worn(ItemKind::LapisSkyIron, std::nullopt)).staminaRegenPercent == 60);
	CHECK(ringBonus(worn(ItemKind::FeldsparSkyIron, std::nullopt)).poisonFasterPercent == 70);
	// Sky iron (plan sky-iron-rings): a whole tier, and it moves immune; two: two tiers, the cap.
	CHECK(ringBonus(worn(ItemKind::AmethystSkyIron, ItemKind::AmethystGold)).amethystPercent == 150);
	CHECK(ringBonus(worn(ItemKind::AmethystSkyIron, ItemKind::AmethystSkyIron)).amethystPercent == 200);
	CHECK(ringBonus(worn(ItemKind::AmethystSkyIron, std::nullopt)).amethystMovesImmune);
	CHECK_FALSE(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystGold)).amethystMovesImmune);
	CHECK(ringBonus(worn(ItemKind::AmethystSilver, ItemKind::AmethystSilver)).amethystPercent == 40);
	CHECK(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystCopper)).amethystPercent == 60);
	CHECK(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystSilver)).amethystPercent == 70);
	CHECK(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystGold)).amethystPercent == 100);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystSilver)), RingGem::Amethyst) ==
		"resistances 70% of a tier weaker"
	);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystGold)), RingGem::Amethyst) ==
		"resistances a tier weaker"
	);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::GarnetGold, ItemKind::GarnetGold)), RingGem::Garnet) ==
		"heals 1 health every 1.5 s"
	);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::GarnetSkyIron, ItemKind::GarnetSkyIron)), RingGem::Garnet) ==
		"heals 2% health every 2 s"
	);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::GarnetSkyIron, ItemKind::GarnetGold)), RingGem::Garnet) ==
		"heals 1% health every 2 s, 1 every 3.0 s"
	);
	CHECK(ringSumText(mixed, RingGem::Jasper).empty());
}

TEST_CASE("a ring card's hand buttons: on, off, swapped, moved over") {
	ItemBag bag = bagWith({ItemKind::GarnetCopper, ItemKind::JasperGold});
	CHECK(bag.WearOn(ItemKind::GarnetCopper, Hand::Left, true));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::GarnetCopper);
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	// The only garnet moves to the other hand.
	CHECK(bag.WearOn(ItemKind::GarnetCopper, Hand::Right, true));
	CHECK_FALSE(bag.RingOn(Hand::Left).has_value());
	CHECK(bag.RingOn(Hand::Right) == ItemKind::GarnetCopper);
	// The jasper on the right: the garnet comes off.
	CHECK(bag.WearOn(ItemKind::JasperGold, Hand::Right, true));
	CHECK(bag.RingOn(Hand::Right) == ItemKind::JasperGold);
	CHECK_FALSE(bag.RingWorn(ItemKind::GarnetCopper));
	// The hand it is on: off.
	CHECK(bag.WearOn(ItemKind::JasperGold, Hand::Right, true));
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	// Not held, or dead: nothing.
	CHECK_FALSE(bag.WearOn(ItemKind::FeldsparGold, Hand::Left, true));
	CHECK_FALSE(bag.WearOn(ItemKind::GarnetCopper, Hand::Left, false));
	CHECK(bag.Block(ItemKind::GarnetCopper, ALIVE) == UseBlock::None);
}

TEST_CASE("two copies of a ring go on both hands, one copy only on one") {
	ItemBag bag = bagWith({ItemKind::GarnetGold, ItemKind::GarnetGold});
	CHECK(bag.WearOn(ItemKind::GarnetGold, Hand::Left, true));
	CHECK(bag.WearOn(ItemKind::GarnetGold, Hand::Right, true));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::GarnetGold);
	CHECK(bag.RingOn(Hand::Right) == ItemKind::GarnetGold);
	// The quick way takes off the one put on last.
	CHECK(bag.Use(ItemKind::GarnetGold, ALIVE));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::GarnetGold);
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
}

TEST_CASE("the quick way: worn comes off, else the free hand, left first, else in place of the older ring") {
	ItemBag bag = bagWith({ItemKind::GarnetCopper, ItemKind::JasperCopper, ItemKind::FeldsparCopper});
	CHECK(bag.Use(ItemKind::GarnetCopper, ALIVE));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::GarnetCopper);
	CHECK(bag.Use(ItemKind::JasperCopper, ALIVE));
	CHECK(bag.RingOn(Hand::Right) == ItemKind::JasperCopper);
	CHECK(bag.Use(ItemKind::FeldsparCopper, ALIVE)); // the garnet went on first
	CHECK(bag.RingOn(Hand::Left) == ItemKind::FeldsparCopper);
	CHECK(bag.RingOn(Hand::Right) == ItemKind::JasperCopper);
	CHECK(bag.Use(ItemKind::GarnetCopper, ALIVE)); // now the jasper is the older
	CHECK(bag.RingOn(Hand::Right) == ItemKind::GarnetCopper);
	CHECK(bag.Use(ItemKind::GarnetCopper, ALIVE)); // worn: off
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	CHECK(bag.Use(ItemKind::JasperCopper, ALIVE)); // the free hand
	CHECK(bag.RingOn(Hand::Right) == ItemKind::JasperCopper);
}

TEST_CASE("two worn rings survive a save and a load; a save from before the rings loads with none") {
	ItemBag bag = bagWith({ItemKind::CrystalSilver, ItemKind::AmethystGold, ItemKind::GarnetCopper});
	bag.WearOn(ItemKind::AmethystGold, Hand::Right, true);
	bag.WearOn(ItemKind::CrystalSilver, Hand::Left, true);
	std::stringstream file;
	bag.Save(file);
	ItemBag loaded;
	loaded.Load(file);
	CHECK(loaded.RingOn(Hand::Left) == ItemKind::CrystalSilver);
	CHECK(loaded.RingOn(Hand::Right) == ItemKind::AmethystGold);
	CHECK(loaded.Count(ItemKind::GarnetCopper) == 1);
	CHECK(loaded.Use(ItemKind::GarnetCopper, ALIVE)); // the older one (the amethyst) gives way
	CHECK(loaded.RingOn(Hand::Right) == ItemKind::GarnetCopper);

	// The same bag as INV7: no rings after the worn amulet.
	std::string line = file.str();
	std::stringstream again;
	bag.Save(again);
	line = again.str();
	line.replace(0, 4, "INV7");
	for (int field = 0; field < 5; field++) // drop the rings' five numbers
		line.erase(line.find_last_of(' '));
	std::stringstream old(line + "\n");
	ItemBag before;
	before.Load(old);
	CHECK(before.Count(ItemKind::AmethystGold) == 1);
	CHECK_FALSE(before.RingOn(Hand::Left).has_value());
	CHECK_FALSE(before.RingOn(Hand::Right).has_value());
}

TEST_CASE("the garnet heals in a fight too, but not while poisoned") {
	PlayerStats stats;
	stats.LoseHP(20);
	const int hp = stats.CurrentHP();
	stats.WearRings(ringBonus(worn(ItemKind::GarnetGold, std::nullopt)));
	for (int ms = 0; ms < 3000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS); // not safe: a monster chases
	CHECK(stats.CurrentHP() == hp + 1);
	stats.vitals.poison.Apply(PoisonTier::Weak);
	for (int ms = 0; ms < 6000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp + 1);
	stats.vitals.poison.Cure();
	stats.WearRings(ringBonus(worn(ItemKind::GarnetGold, ItemKind::GarnetGold)));
	for (int ms = 0; ms < 3000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp + 3); // two: 1 every 1.5 s
}

TEST_CASE("the sky-iron garnet heals 1% of max HP every 2 s, at least 1 (plan sky-iron-rings)") {
	auto healed = [](int maxHpPercent, std::optional<ItemKind> second) {
		PlayerStats stats; // 50 max HP
		stats.AddMaxHP(maxHpPercent);
		stats.LoseHP(stats.CurrentMaxHP() - 1);
		stats.WearRings(ringBonus(worn(ItemKind::GarnetSkyIron, second)));
		for (int ms = 0; ms < 20000; ms += UPDATE_TICK_MS)
			stats.Regenerate(false, UPDATE_TICK_MS);
		return stats.CurrentHP() - 1;
	};
	CHECK(healed(0, std::nullopt) == 10); // 50 max HP, under 100: 1 every 2 s
	CHECK(healed(500, std::nullopt) == 30); // 300 max HP: 3 every 2 s
	CHECK(healed(500, ItemKind::GarnetSkyIron) == 60); // two: twice
}

TEST_CASE("the feldspar runs the poison out sooner, its bite a second the same") {
	Poison plain;
	Poison washed;
	plain.Apply(PoisonTier::Weak);
	washed.Apply(PoisonTier::Weak);
	int plainHp = 0;
	int washedHp = 0;
	for (int ms = 0; ms < POISON_TIERS[0].durationMs; ms += UPDATE_TICK_MS) {
		plainHp += plain.Advance(UPDATE_TICK_MS, 100);
		washedHp += washed.Advance(UPDATE_TICK_MS, 100, 50);
		if (ms < 10000)
			CHECK(plainHp - washedHp <= 1); // the same damage a second while both run
	}
	CHECK_FALSE(washed.Any());
	// 20 s of weak poison at 1 HP a second, run down half again as fast: about 13 s.
	CHECK(plainHp == 20);
	CHECK(washedHp >= 12);
	CHECK(washedHp <= 14);
	PlayerStats stats;
	stats.WearRings(ringBonus(worn(ItemKind::FeldsparGold, ItemKind::FeldsparCopper)));
	CHECK(stats.PoisonFasterPercent() == 70);
}

TEST_CASE("the lapis brings the stamina back faster") {
	GameClock::enableVirtual(); // alone in its shard: advance moves nothing on the real clock
	auto regained = [](std::optional<ItemKind> ring) {
		PlayerStats stats;
		stats.WearRings(ringBonus(worn(ring, std::nullopt)));
		stats.SetStamina(0);
		WorldEvents events;
		for (int ms = 0; ms < 4000; ms += UPDATE_TICK_MS) {
			GameClock::advance(UPDATE_TICK_MS);
			stats.UpdateStamina(events);
		}
		return stats.Stamina();
	};
	const int plain = regained(std::nullopt);
	REQUIRE(plain > 0);
	CHECK(regained(ItemKind::LapisGold) == doctest::Approx(plain * 1.4).epsilon(0.1));
}

TEST_CASE("the jasper walks faster on the ground, the jump carries as far as without it") {
	CHECK(
		walked(ItemKind::JasperGold, 2000, false) ==
		doctest::Approx(walked(std::nullopt, 2000, false) * 1.12f).epsilon(0.03)
	);
	CHECK(walked(ItemKind::JasperGold, 0, true) == doctest::Approx(walked(std::nullopt, 0, true)));
	PlayerStats stats;
	stats.WearRings(ringBonus(worn(ItemKind::JasperGold, ItemKind::JasperCopper)));
	CHECK(stats.WalkFactor(false) == doctest::Approx(1.17f));
	CHECK(stats.WalkFactor(true) == doctest::Approx(1.f));
	stats.curses.Lay(Curse::Slowness, CurseGrade::Lesser, 1000); // the slowness halves the faster walk
	CHECK(stats.WalkFactor(false) == doctest::Approx(1.17f * SLOWNESS_WALK_FACTOR));
}

TEST_CASE("the rock crystal reaches farther, melee and ranged, times the Sharp eye") {
	PlayerStats stats;
	stats.WearRings(ringBonus(worn(ItemKind::CrystalGold, std::nullopt)));
	CHECK(stats.WeaponReach(ItemKind::Dagger) == doctest::Approx(weaponReach(ItemKind::Dagger) * 1.25f));
	CHECK(stats.WeaponReach(ItemKind::SelfBow) == doctest::Approx(weaponReach(ItemKind::SelfBow) * 1.25f));
	stats.heka.Earn(5);
	REQUIRE(stats.heka.Learn(SkillId::SharpEye, true));
	CHECK(stats.WeaponReach(ItemKind::SelfBow) == doctest::Approx(weaponReach(ItemKind::SelfBow) * 1.3f * 1.25f));
}

TEST_CASE("the amethyst moves each rate a share of a tier up the ladder (plan resistance-tiers)") {
	// The plan's table: copper 10%, silver 20%, gold 50%, two gold 100% (a whole tier).
	const Resistances tiers = {TOUGH, RESISTS, NORMAL, WEAK};
	CHECK(amethystResist(tiers, 10) == Resistances{27.5f, 55.f, 105.f, 155.f});
	CHECK(amethystResist(tiers, 20) == Resistances{30.f, 60.f, 110.f, 160.f});
	CHECK(amethystResist(tiers, 50) == Resistances{37.5f, 75.f, 125.f, 175.f});
	CHECK(amethystResist(tiers, 100) == Resistances{RESISTS, NORMAL, WEAK, VULNERABLE});
	// Gold and silver: 70%; over a whole tier it goes on into the next; vulnerable is the top.
	CHECK(amethystResist(tiers, 70) == Resistances{42.5f, 85.f, 135.f, 185.f});
	CHECK(amethystResist(tiers, 150) == Resistances{75.f, 125.f, 175.f, VULNERABLE});
	CHECK(amethystResist(tiers, 200) == Resistances{NORMAL, WEAK, VULNERABLE, VULNERABLE});
	CHECK(amethystResist({VULNERABLE, VULNERABLE, VULNERABLE, VULNERABLE}, 50)[0] == VULNERABLE);
	// Immune stays immune: no copper, silver or gold ring moves it.
	CHECK(amethystResist({IMMUNE, TOUGH, NORMAL, NORMAL}, 200)[0] == IMMUNE);
	// Only a sky iron does (plan sky-iron-rings): one a tier, with a gold half a tier more, two of them two tiers.
	CHECK(
		amethystResist({IMMUNE, TOUGH, NORMAL, VULNERABLE}, 100, true) == Resistances{TOUGH, RESISTS, WEAK, VULNERABLE}
	);
	CHECK(amethystResist({IMMUNE, TOUGH, NORMAL, NORMAL}, 150, true)[0] == 37.5f);
	CHECK(amethystResist({IMMUNE, TOUGH, NORMAL, NORMAL}, 150, true)[1] == 75.f);
	CHECK(amethystResist({IMMUNE, TOUGH, NORMAL, NORMAL}, 200, true)[0] == RESISTS);

	// A sword on a giant scarab (it resists a cut): more damage with the gold amethyst, as the player's hit.
	auto dealt = [](std::optional<ItemKind> ring) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/hitbox_giant_scarab"));
		if (ring)
			world.WearRing(*ring, Hand::Left);
		REQUIRE(world.WalkTo(world.X() + 1.f));
		world.Wait(12000);
		const int before = world.dungeon.NearestMonsterHealth();
		REQUIRE(world.dungeon.AttackNearest(35, weaponMix(ItemKind::ShortSword), weaponReach(ItemKind::ShortSword), 1));
		return before - world.dungeon.NearestMonsterHealth();
	};
	CHECK(dealt(std::nullopt) == 25); // 35 x (85% x 50 + 15% x 200)
	CHECK(dealt(ItemKind::AmethystGold) == 33); // 35 x (85% x 75 + 15% x 200)
}

TEST_CASE("chests hold a ring now and then: the metal by depth, the amethyst rarer") {
	Rng rng(17);
	int rings = 0;
	for (int i = 0; i < 4000; i++)
		for (ItemKind item : RollChestLoot(ItemKind::SmallHealth, OwnedWeapons{}, 1, rng))
			rings += isRing(item) ? 1 : 0;
	CHECK(rings > 4000 * CHEST_RING_CHANCE / 100 * 8 / 10);
	CHECK(rings < 4000 * CHEST_RING_CHANCE / 100 * 12 / 10);

	auto metals = [](int level) {
		Rng r(23);
		std::array<int, RING_METAL_COUNT> count{};
		std::array<int, RING_GEM_COUNT> gems{};
		for (int i = 0; i < 6000; i++) {
			const Ring ring = ringOf(RollRing(level, r));
			count[static_cast<size_t>(ring.metal)]++;
			gems[static_cast<size_t>(ring.gem)]++;
		}
		const int amethyst = gems[static_cast<size_t>(RingGem::Amethyst)];
		CHECK(amethyst > gems[static_cast<size_t>(RingGem::Garnet)] / 3);
		CHECK(amethyst < gems[static_cast<size_t>(RingGem::Garnet)] * 2 / 3); // half the weight
		return count;
	};
	const auto early = metals(1);
	CHECK(early[1] == 0);
	CHECK(early[2] == 0);
	const auto middle = metals(SILVER_RING_LEVEL);
	CHECK(middle[1] > 0);
	CHECK(middle[2] == 0);
	const auto deep = metals(GOLD_RING_LEVEL);
	CHECK(deep[2] > 0);
	CHECK(deep[2] < deep[1]);
}

TEST_CASE("the ring carriers drop a ring now and then, bosses their amulet") {
	CHECK(monsterKind(MonsterMummy)->ringDropPercent > 0);
	CHECK(monsterKind(MonsterAnubis)->ringDropPercent > 0);
	CHECK(monsterKind(MonsterCursedMimic)->ringDropPercent > 0);
	Rng rng(29);
	int rings = 0;
	for (int i = 0; i < 2000; i++) {
		const std::optional<ItemKind> drop = RollKillDrop(false, OwnedWeapons{}, 20, rng, 10);
		rings += drop && isRing(*drop) ? 1 : 0;
		const std::optional<ItemKind> boss = RollKillDrop(true, OwnedWeapons{}, 20, rng, 10);
		REQUIRE(boss.has_value());
		CHECK(isAmulet(*boss));
	}
	CHECK(rings > 140);
	CHECK(rings < 260);
}

TEST_CASE("the first ring of each gem writes its field note") {
	for (int g = 0; g < RING_GEM_COUNT; g++) {
		const auto gem = static_cast<RingGem>(g);
		CHECK(std::string(fieldNoteText(ringNote(gem)).title) != "");
		CHECK(std::string(fieldNoteText(ringNote(gem)).text) != "");
	}
	CHECK(static_cast<int>(FieldNote::RingLast) + 1 == static_cast<int>(FieldNote::ShrineOfIsis)); // saved by number
	Journal journal;
	ItemBag bag;
	bag.Find(ItemKind::JasperSilver, journal);
	bag.Find(ItemKind::JasperGold, journal);
	REQUIRE(journal.Notes().size() == 1);
	CHECK(journal.Notes()[0] == ringNote(RingGem::Jasper));
}

TEST_CASE("the first sky-iron ring writes the note Sky iron, once (plan sky-iron-rings)") {
	CHECK(std::string(fieldNoteText(FieldNote::SkyIron).title) == "Sky iron");
	Journal journal;
	ItemBag bag;
	bag.Find(ItemKind::GarnetGold, journal);
	bag.Find(ItemKind::GarnetSkyIron, journal);
	bag.Find(ItemKind::LapisSkyIron, journal);
	CHECK(journal.Notes() == std::vector{ringNote(RingGem::Garnet), FieldNote::SkyIron, ringNote(RingGem::Lapis)});
}
