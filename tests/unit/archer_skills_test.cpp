// Plan archer-skills: Arrows of Satet (a bow shot cuts 5 / 10 deeper and may bleed, medium from rank II), Sting of
// Serket (a bow hit may poison, medium / strong) and Fang of Wadjet (a blade's hit, the same), stacking with the venom
// amulet and the Bleed skill as one roll.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/archer_skills.h"
#include "../../src/level/gameplay_shared.h"

namespace {
// tests/levels/hitbox_bow: a giant rat (no poison resistance, bleeds) 2 tiles off, the player with `skill` at `rank`.
Monster* ratWith(SimWorld& world, SkillId skill, int rank) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_bow"));
	Heka& heka = world.player.stats.heka;
	for (int r = 0; r < rank; r++) {
		heka.Earn(5 + r);
		REQUIRE((r == 0 ? heka.Learn(skill) : heka.Upgrade(skill)));
	}
	REQUIRE(world.WalkTo(world.X() + 3.5f));
	world.Wait(500);
	for (int i = 0; i < MAX_MONSTERS; i++) {
		auto* mon = const_cast<Monster*>(&world.dungeon.Monsters()[i]);
		if (mon->Active() && mon->Alive())
			return mon;
	}
	FAIL("no monster");
	return nullptr;
}

// A shot of kind (0 damage, the bow's mix) until `done`, at most `shots`: how many it took (shots + 1: never).
template <typename Done> int shootUntil(SimWorld& world, Monster& rat, MissileKind kind, int shots, Done done) {
	for (int i = 1; i <= shots; i++) {
		rat.RestoreHealth(rat.MaxHealth());
		world.dungeon.Shoot(
			kind,
			1,
			weaponDef(ItemKind::SelfBow).mix,
			1,
			0.5f * world.player.Height(),
			weaponReach(ItemKind::SelfBow)
		);
		world.Wait(700);
		if (done(rat))
			return i;
	}
	return shots + 1;
}

// A blade's hit of no damage until `done`, at most `hits`.
template <typename Done> int hitUntil(SimWorld& world, Monster& rat, int hits, Done done) {
	for (int i = 1; i <= hits; i++) {
		rat.RestoreHealth(rat.MaxHealth());
		REQUIRE(world.dungeon.AttackNearest(0, {0, 100, 0}, 3.f, 1));
		if (done(rat))
			return i;
	}
	return hits + 1;
}
} // namespace

TEST_CASE("archer-skills: the cut of a bow shot is 5 at rank I, 10 at rank II, all of it cut") {
	CHECK(sharpArrowCut(0) == 0);
	CHECK(sharpArrowCut(1) == 5);
	CHECK(sharpArrowCut(2) == 10);
	DamageMix mix = {0, 0, 100}; // a bow: all stab
	CHECK(addCut(7, mix, 5) == 12);
	CHECK(mix[std::to_underlying(DamageType::Pierce)] == 58); // 7 of 12
	CHECK(mix[std::to_underlying(DamageType::Slash)] == 42); // 5 of 12
	CHECK(mix[0] + mix[1] + mix[2] + mix[3] == 100);
	DamageMix none = {0, 0, 100};
	CHECK(addCut(7, none, 0) == 7);
	CHECK(none[std::to_underlying(DamageType::Pierce)] == 100);
	DamageMix mixed = {10, 20, 50, 20};
	CHECK(addCut(22, mixed, 10) == 32);
	CHECK(mixed[0] + mixed[1] + mixed[2] + mixed[3] == 100);
	CHECK(mixed[std::to_underlying(DamageType::Slash)] > 20);
}

TEST_CASE("archer-skills: Arrows of Satet's bleed starts weak at rank I, medium at rank II, a boss at half") {
	CHECK(sharpBleedPercent(0, 0, false) == 0);
	CHECK(sharpBleedPercent(1, 0, false) == 20);
	CHECK(sharpBleedPercent(1, 1, false) == 10);
	CHECK(sharpBleedPercent(1, 2, false) == 5);
	CHECK(sharpBleedPercent(2, 0, false) == 0); // starts at medium
	CHECK(sharpBleedPercent(2, 1, false) == 20);
	CHECK(sharpBleedPercent(2, 2, false) == 10);
	CHECK(sharpBleedPercent(1, 0, true) == 10);
	CHECK(sharpBleedStart(1) == 0);
	CHECK(sharpBleedStart(2) == 1);
	CHECK(sharpBleedStart(0) == -1);
}

TEST_CASE(
	"archer-skills: the poison skills and the venom amulet roll as one, the chances add, the stronger tier counts"
) {
	CHECK(poisonSkillPercent(0) == 0);
	CHECK(poisonSkillPercent(1) == 20);
	CHECK(poisonSkillPercent(2) == 20); // the same chance at both ranks
	CHECK(poisonSkillTier(1) == PoisonTier::Medium);
	CHECK(poisonSkillTier(2) == PoisonTier::Strong);
	auto skillOnly = combinedPoison(0, PoisonTier::Weak, 1);
	CHECK(skillOnly.percent == 20);
	CHECK(skillOnly.tier == PoisonTier::Medium);
	auto amuletOnly = combinedPoison(20, PoisonTier::Weak, 0);
	CHECK(amuletOnly.percent == 20);
	CHECK(amuletOnly.tier == PoisonTier::Weak); // not the skill's medium
	auto both = combinedPoison(20, PoisonTier::Weak, 1);
	CHECK(both.percent == 40);
	CHECK(both.tier == PoisonTier::Medium);
	CHECK(combinedPoison(20, PoisonTier::Strong, 1).tier == PoisonTier::Strong);
	CHECK(combinedPoison(20, PoisonTier::Weak, 2).tier == PoisonTier::Strong);
	CHECK(combinedPoison(90, PoisonTier::Weak, 2).percent == 100);
	CHECK(combinedPoison(0, PoisonTier::Weak, 0).percent == 0);
}

TEST_CASE("archer-skills: the three skills are learnt and upgraded, each with a field note") {
	for (SkillId id : {SkillId::SharpArrows, SkillId::PoisonArrows, SkillId::PoisonBlade}) {
		CHECK(upgradable(id));
		CHECK(skillNote(id).has_value());
		CHECK(std::string(skillRankTwoLore(id)).size() > 0);
	}
	CHECK(SKILL_COUNT == 21);
}

TEST_CASE("archer-skills: a bow shot cuts deeper with Arrows of Satet, more at rank II") {
	int lost[3] = {0, 0, 0};
	for (int rank = 0; rank < 3; rank++) {
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, rank);
		const int before = rat->Health();
		world.dungeon.Shoot(
			MissileKind::Arrow,
			7,
			weaponDef(ItemKind::SelfBow).mix,
			1,
			0.5f * world.player.Height(),
			weaponReach(ItemKind::SelfBow)
		);
		world.Wait(1500);
		lost[rank] = before - rat->Health();
	}
	CHECK(lost[1] > lost[0]);
	CHECK(lost[2] > lost[1]);
}

TEST_CASE("archer-skills: only a bow's arrow cuts deeper; a sling stone does not") {
	int lost[2] = {0, 0};
	for (int skilled = 0; skilled < 2; skilled++) {
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, skilled);
		const int before = rat->Health();
		world.dungeon.Shoot(
			MissileKind::Stone,
			7,
			weaponDef(ItemKind::SelfBow).mix,
			1,
			0.5f * world.player.Height(),
			weaponReach(ItemKind::SelfBow)
		);
		world.Wait(1500);
		lost[skilled] = before - rat->Health();
	}
	CHECK(lost[0] > 0);
	CHECK(lost[1] == lost[0]);
}

TEST_CASE("archer-skills: a bow hit bleeds weak at rank I; at rank II only medium; a stone never; no skill never") {
	{
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, 1);
		CHECK(shootUntil(world, *rat, MissileKind::Arrow, 150, [](const Monster& m) { return m.Bleeds(); }) <= 150);
		CHECK((rat->BleedMask() & 1) != 0); // weak
	}
	{
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, 2);
		CHECK(shootUntil(world, *rat, MissileKind::Arrow, 150, [](const Monster& m) { return m.Bleeds(); }) <= 150);
		CHECK((rat->BleedMask() & 2) != 0); // medium
		CHECK((rat->BleedMask() & 1) == 0); // not weak
	}
	{
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, 2);
		CHECK(shootUntil(world, *rat, MissileKind::Stone, 60, [](const Monster& m) { return m.Bleeds(); }) == 61);
	}
	{
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::SharpArrows, 0);
		CHECK(shootUntil(world, *rat, MissileKind::Arrow, 60, [](const Monster& m) { return m.Bleeds(); }) == 61);
	}
}

TEST_CASE(
	"archer-skills: Sting of Serket poisons a bow hit, medium at rank I, strong at rank II; not a stone or a blade"
) {
	for (int rank = 1; rank <= 2; rank++) {
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::PoisonArrows, rank);
		CHECK(shootUntil(world, *rat, MissileKind::Arrow, 150, [](const Monster& m) { return m.Poisoned(); }) <= 150);
		CHECK(rat->PoisonMask() == (rank == 1 ? 2 : 4));
	}
	SimWorld world;
	Monster* rat = ratWith(world, SkillId::PoisonArrows, 2);
	CHECK(shootUntil(world, *rat, MissileKind::Stone, 60, [](const Monster& m) { return m.Poisoned(); }) == 61);
	CHECK(hitUntil(world, *rat, 60, [](const Monster& m) { return m.Poisoned(); }) == 61); // a blade: no
}

TEST_CASE("archer-skills: Fang of Wadjet poisons a blade's hit, medium at rank I, strong at rank II; not an arrow") {
	for (int rank = 1; rank <= 2; rank++) {
		SimWorld world;
		Monster* rat = ratWith(world, SkillId::PoisonBlade, rank);
		CHECK(hitUntil(world, *rat, 150, [](const Monster& m) { return m.Poisoned(); }) <= 150);
		CHECK(rat->PoisonMask() == (rank == 1 ? 2 : 4));
	}
	SimWorld world;
	Monster* rat = ratWith(world, SkillId::PoisonBlade, 2);
	CHECK(shootUntil(world, *rat, MissileKind::Arrow, 60, [](const Monster& m) { return m.Poisoned(); }) == 61);
}

TEST_CASE("archer-skills: no skill, no poison") {
	SimWorld world;
	Monster* rat = ratWith(world, SkillId::PoisonBlade, 0);
	CHECK(hitUntil(world, *rat, 80, [](const Monster& m) { return m.Poisoned(); }) == 81);
}
