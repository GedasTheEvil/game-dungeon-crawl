// The rings (plan ring-accessories): two worn at once, one per hand; the gem's effect, the metal's strength, the sums
// and caps, the hands, the save, the loot.
#include "../../external/doctest/doctest.h"
#include "../../src/entities/player_stats.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/journal.h"
#include "../../src/world/journal_text.h"
#include "../../src/world/loot.h"
#include "../../src/world/movement.h"
#include "sim_world.h"
#include <sstream>
#include <string>

namespace {
constexpr Vitals ALIVE{true, 1, 1, 1, 1};

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

TEST_CASE("rings: six gems, three metals, named by both") {
	CHECK(isRing(ItemKind::RubyCopper));
	CHECK_FALSE(isAmulet(ItemKind::RubyCopper));
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
	CHECK(std::string(itemText(ItemKind::RubyGold).name) == "Gold Ruby Ring");
	CHECK(std::string(itemText(ItemKind::RubyGold).shortName) == "Ruby III");
	CHECK(std::string(itemText(ItemKind::RubyGold).label) == "gold ring of regeneration");
	CHECK(std::string(itemText(ItemKind::RubyGold).effect) == "Heals 1 health every 3 s");
	CHECK(std::string(itemText(ItemKind::AmethystCopper).shortName) == "Amethyst I");
	CHECK(std::string(itemText(ItemKind::TopazSilver).effect) == "Walk and sprint 8% faster");
	CHECK(std::string(itemText(ItemKind::CrystalGold).lore1) != "");
}

TEST_CASE("the ring bonus: both hands count, the same gem stacks, two identical rings too, up to the caps") {
	CHECK(ringBonus(worn(std::nullopt, std::nullopt)).regenHpPerHour == 0);
	const RingBonus mixed = ringBonus(worn(ItemKind::SapphireSilver, ItemKind::EmeraldGold));
	CHECK(mixed.staminaRegenPercent == 25);
	CHECK(mixed.poisonFasterPercent == 50);
	CHECK(mixed.speedPercent == 0);
	// Two gold rubies: 1 HP every 1.5 s.
	CHECK(ringBonus(worn(ItemKind::RubyGold, ItemKind::RubyGold)).regenHpPerHour == 2 * RingEffect::MS_PER_HOUR / 3000);
	CHECK(ringBonus(worn(ItemKind::TopazCopper, ItemKind::TopazCopper)).speedPercent == 10);
	CHECK(ringBonus(worn(ItemKind::TopazCopper, ItemKind::TopazSilver)).speedPercent == 13);
	CHECK(ringBonus(worn(ItemKind::TopazGold, ItemKind::TopazGold)).speedPercent == RingEffect::TOPAZ_CAP);
	CHECK(ringBonus(worn(ItemKind::CrystalGold, ItemKind::CrystalGold)).reachPercent == RingEffect::CRYSTAL_CAP);
	CHECK(ringBonus(worn(ItemKind::AmethystSilver, ItemKind::AmethystSilver)).resistCutPercent == 100);
	CHECK(ringBonus(worn(ItemKind::AmethystGold, ItemKind::AmethystSilver)).resistCutPercent == 100);
	CHECK(
		ringSumText(ringBonus(worn(ItemKind::RubyGold, ItemKind::RubyGold)), RingGem::Ruby) ==
		"heals 1 health every 1.5 s"
	);
	CHECK(ringSumText(mixed, RingGem::Topaz).empty());
}

TEST_CASE("a ring card's hand buttons: on, off, swapped, moved over") {
	ItemBag bag = bagWith({ItemKind::RubyCopper, ItemKind::TopazGold});
	CHECK(bag.WearOn(ItemKind::RubyCopper, Hand::Left, true));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::RubyCopper);
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	// The only ruby moves to the other hand.
	CHECK(bag.WearOn(ItemKind::RubyCopper, Hand::Right, true));
	CHECK_FALSE(bag.RingOn(Hand::Left).has_value());
	CHECK(bag.RingOn(Hand::Right) == ItemKind::RubyCopper);
	// The topaz on the right: the ruby comes off.
	CHECK(bag.WearOn(ItemKind::TopazGold, Hand::Right, true));
	CHECK(bag.RingOn(Hand::Right) == ItemKind::TopazGold);
	CHECK_FALSE(bag.RingWorn(ItemKind::RubyCopper));
	// The hand it is on: off.
	CHECK(bag.WearOn(ItemKind::TopazGold, Hand::Right, true));
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	// Not held, or dead: nothing.
	CHECK_FALSE(bag.WearOn(ItemKind::EmeraldGold, Hand::Left, true));
	CHECK_FALSE(bag.WearOn(ItemKind::RubyCopper, Hand::Left, false));
	CHECK(bag.Block(ItemKind::RubyCopper, ALIVE) == UseBlock::None);
}

TEST_CASE("two copies of a ring go on both hands, one copy only on one") {
	ItemBag bag = bagWith({ItemKind::RubyGold, ItemKind::RubyGold});
	CHECK(bag.WearOn(ItemKind::RubyGold, Hand::Left, true));
	CHECK(bag.WearOn(ItemKind::RubyGold, Hand::Right, true));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::RubyGold);
	CHECK(bag.RingOn(Hand::Right) == ItemKind::RubyGold);
	// The quick way takes off the one put on last.
	CHECK(bag.Use(ItemKind::RubyGold, ALIVE));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::RubyGold);
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
}

TEST_CASE("the quick way: worn comes off, else the free hand, left first, else in place of the older ring") {
	ItemBag bag = bagWith({ItemKind::RubyCopper, ItemKind::TopazCopper, ItemKind::EmeraldCopper});
	CHECK(bag.Use(ItemKind::RubyCopper, ALIVE));
	CHECK(bag.RingOn(Hand::Left) == ItemKind::RubyCopper);
	CHECK(bag.Use(ItemKind::TopazCopper, ALIVE));
	CHECK(bag.RingOn(Hand::Right) == ItemKind::TopazCopper);
	CHECK(bag.Use(ItemKind::EmeraldCopper, ALIVE)); // the ruby went on first
	CHECK(bag.RingOn(Hand::Left) == ItemKind::EmeraldCopper);
	CHECK(bag.RingOn(Hand::Right) == ItemKind::TopazCopper);
	CHECK(bag.Use(ItemKind::RubyCopper, ALIVE)); // now the topaz is the older
	CHECK(bag.RingOn(Hand::Right) == ItemKind::RubyCopper);
	CHECK(bag.Use(ItemKind::RubyCopper, ALIVE)); // worn: off
	CHECK_FALSE(bag.RingOn(Hand::Right).has_value());
	CHECK(bag.Use(ItemKind::TopazCopper, ALIVE)); // the free hand
	CHECK(bag.RingOn(Hand::Right) == ItemKind::TopazCopper);
}

TEST_CASE("two worn rings survive a save and a load; a save from before the rings loads with none") {
	ItemBag bag = bagWith({ItemKind::CrystalSilver, ItemKind::AmethystGold, ItemKind::RubyCopper});
	bag.WearOn(ItemKind::AmethystGold, Hand::Right, true);
	bag.WearOn(ItemKind::CrystalSilver, Hand::Left, true);
	std::stringstream file;
	bag.Save(file);
	ItemBag loaded;
	loaded.Load(file);
	CHECK(loaded.RingOn(Hand::Left) == ItemKind::CrystalSilver);
	CHECK(loaded.RingOn(Hand::Right) == ItemKind::AmethystGold);
	CHECK(loaded.Count(ItemKind::RubyCopper) == 1);
	CHECK(loaded.Use(ItemKind::RubyCopper, ALIVE)); // the older one (the amethyst) gives way
	CHECK(loaded.RingOn(Hand::Right) == ItemKind::RubyCopper);

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

TEST_CASE("the ruby heals in a fight too, but not while poisoned") {
	PlayerStats stats;
	stats.LoseHP(20);
	const int hp = stats.CurrentHP();
	stats.WearRings(ringBonus(worn(ItemKind::RubyGold, std::nullopt)));
	for (int ms = 0; ms < 3000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS); // not safe: a monster chases
	CHECK(stats.CurrentHP() == hp + 1);
	stats.poison.Apply(PoisonTier::Weak);
	for (int ms = 0; ms < 6000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp + 1);
	stats.poison.Cure();
	stats.WearRings(ringBonus(worn(ItemKind::RubyGold, ItemKind::RubyGold)));
	for (int ms = 0; ms < 3000; ms += UPDATE_TICK_MS)
		stats.Regenerate(false, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp + 3); // two: 1 every 1.5 s
}

TEST_CASE("the emerald runs the poison out sooner, its bite a second the same") {
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
	stats.WearRings(ringBonus(worn(ItemKind::EmeraldGold, ItemKind::EmeraldCopper)));
	CHECK(stats.PoisonFasterPercent() == 70);
}

TEST_CASE("the sapphire brings the stamina back faster") {
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
	CHECK(regained(ItemKind::SapphireGold) == doctest::Approx(plain * 1.4).epsilon(0.1));
}

TEST_CASE("the topaz walks faster on the ground, the jump carries as far as without it") {
	CHECK(
		walked(ItemKind::TopazGold, 2000, false) ==
		doctest::Approx(walked(std::nullopt, 2000, false) * 1.12f).epsilon(0.03)
	);
	CHECK(walked(ItemKind::TopazGold, 0, true) == doctest::Approx(walked(std::nullopt, 0, true)));
	PlayerStats stats;
	stats.WearRings(ringBonus(worn(ItemKind::TopazGold, ItemKind::TopazCopper)));
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

TEST_CASE("the amethyst cuts through resistances, weaknesses stay") {
	const Resistances scarab = {NORMAL, RESISTS, WEAK, TOUGH};
	CHECK(cutResistances(scarab, 25) == Resistances{NORMAL, 62, WEAK, 43});
	CHECK(cutResistances(scarab, 50) == Resistances{NORMAL, 75, WEAK, 62});
	CHECK(cutResistances(scarab, 100) == Resistances{NORMAL, NORMAL, WEAK, NORMAL});

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
	CHECK(dealt(ItemKind::AmethystGold) == 40); // 35 x (85% x 100 + 15% x 200)
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
		CHECK(amethyst > gems[static_cast<size_t>(RingGem::Ruby)] / 3);
		CHECK(amethyst < gems[static_cast<size_t>(RingGem::Ruby)] * 2 / 3); // half the weight
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
	CHECK(static_cast<int>(FieldNote::RingLast) + 1 == FIELD_NOTE_COUNT); // the last notes, saved by number
	Journal journal;
	ItemBag bag;
	bag.Find(ItemKind::TopazSilver, journal);
	bag.Find(ItemKind::TopazGold, journal);
	REQUIRE(journal.Notes().size() == 1);
	CHECK(journal.Notes()[0] == ringNote(RingGem::Topaz));
}
