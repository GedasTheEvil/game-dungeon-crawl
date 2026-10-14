// Monster rules on the sim harness (plan scenarios-to-unit-tests technique 7), ported from the scenarios that checked
// them with `expect` lines only: courage and traps, speed, leaps, poison, damage types, the health bars.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

TEST_CASE("a reckless mummy sets off a rock fall and, too slow for its middle, is only grazed") {
	SimWorld world; // tests/levels/reckless_rock: the mummy across a rock fall from the player
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_rock"));
	world.player.stats.AddXP(200000, world.events);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.95f));
	world.Wait(2000);
	CHECK(world.Nearest() == 150);
	world.Wait(8000);
	CHECK(world.Nearest() == 125); // half of 50 (trapDamagePct 50)
}

TEST_CASE("a reckless mummy stands in the spikes to strike; the ramp kills it, a trap's kill gives no XP") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_spikes"));
	world.player.stats.AddXP(200000, world.events);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.95f));
	world.Wait(2000);
	CHECK(world.Nearest() == 150);
	world.Wait(7000);
	CHECK(world.Nearest() < 150);
	CHECK(world.Nearest() > 0);
	world.Wait(30000);
	CHECK(world.Nearest() == 0);
	CHECK(world.Xp() == 200000);
}

TEST_CASE("the Anubis walks through the spikes to the player, taking a quarter of their damage") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_anubis"));
	world.player.stats.AddXP(200000, world.events);
	world.WaitTicks(30);
	CHECK(world.Nearest() == 600); // before the spikes at col 4
	REQUIRE(world.WalkTo(2.3f));
	world.Wait(5000);
	CHECK(world.Nearest() < 600);
	CHECK(world.Nearest() > 0);
	CHECK(world.Xp() == 200000);
}

TEST_CASE("the giant rat outruns the player: neither the bow nor walking away keeps it off") {
	SimWorld world; // tests/levels/giant_rat_speed: the rat 4 tiles behind
	REQUIRE(world.Load("tests/levels/giant_rat_speed"));
	world.Equip(ItemKind::SelfBow);
	REQUIRE(world.Walk(-0.1f));
	for (int shot = 0; shot < 3; shot++) {
		world.Attack();
		world.Wait(1000);
	}
	CHECK(world.Nearest() > 0);
	CHECK(world.Hp() < 50);

	REQUIRE(world.Load("tests/levels/giant_rat_speed"));
	REQUIRE(world.Walk(12.f));
	CHECK(world.Hp() < 50); // it gains a quarter tile a second
	CHECK(world.player.Alive());
}

TEST_CASE("walkers stop at pits and traps: the rats on both sides never reach the player") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/monster_hazards"));
	world.Wait(25000);
	CHECK(world.Hp() == 50);
	CHECK(world.player.Alive());
}

TEST_CASE("the giant rat leaps spikes and a pit, 2 s apart, but not a three-cell gap") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/giant_rat_jump"));
	world.Wait(300);
	CHECK(world.Saw(MonsterGiantRat, CreatureMove::Leap)); // the spikes at once
	world.Wait(1600);
	CHECK(world.Hp() == 50); // one cell short of the player, waiting out the cooldown
	world.Wait(2000);
	CHECK(world.Hp() < 50); // over the pit, and bites
	CHECK(world.player.Alive());

	REQUIRE(world.Load("tests/levels/giant_rat_wide"));
	world.Wait(20000);
	CHECK(world.Hp() == 50);
}

TEST_CASE("the giant scarab walks up to the spikes, leaps them, waits at the pit, leaps it and bites") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/giant_scarab_jump"));
	world.Wait(1750);
	CHECK(world.Hp() == 50);
	world.Wait(3750);
	CHECK(world.Saw(MonsterGiantScarab, CreatureMove::Leap));
	world.Wait(1500);
	CHECK(world.Hp() < 50);
}

TEST_CASE("poison runs on a monster as on the player and can kill it; the player's poison kill gives the XP") {
	SimWorld world;
	world.player.god = true;
	// a rat (12 HP, no resistance), weak poison: 1 HP a second
	REQUIRE(world.Load("tests/levels/hitbox_rat"));
	REQUIRE(world.Walk(1.f));
	world.Wait(8000);
	CHECK(world.Nearest() == 12);
	world.dungeon.PoisonNearestMonster(PoisonTier::Weak);
	CHECK(world.dungeon.NearestMonsterPoison() == 1);
	world.Wait(2000);
	CHECK(world.Nearest() == 10);
	CHECK(world.Xp() == 0);
	world.Wait(11000);
	CHECK(world.Nearest() == 0);
	CHECK(world.dungeon.NearestMonsterPoison() == 0);
	CHECK(world.Xp() == 300);

	// the mummy shrugs off any poison
	REQUIRE(world.Load("tests/levels/mummy"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(9.1f));
	world.Wait(2000);
	CHECK(world.Nearest() > 0);
	world.dungeon.PoisonNearestMonster(PoisonTier::Strong);
	CHECK(world.dungeon.NearestMonsterPoison() == 0);

	// a boss (Sobek, no resistance): strong poison, 5% of 1600 HP a second
	REQUIRE(world.Load("tests/levels/sobek"));
	REQUIRE(world.WalkTo(10.8f));
	world.Wait(300);
	CHECK(world.dungeon.BossHealth() == 1600);
	world.dungeon.PoisonBoss(PoisonTier::Strong);
	world.Wait(2000);
	CHECK(world.dungeon.BossHealth() == 1440);
}

TEST_CASE("damage types: blunt normal, blade turned, the point a weakness; the first hit of each is noted, and kept") {
	SimWorld world; // the giant scarab: club 10 * 0.925 = 9, sword 35 * 0.725 = 25, spear 20 * 1.775 = 36
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_giant_scarab"));
	world.Give(ItemKind::ShortSword);
	world.Give(ItemKind::Spear);
	world.Equip(ItemKind::Club);
	REQUIRE(world.Walk(1.f));
	world.Wait(12000);
	CHECK(world.Nearest() == 90);
	world.Attack();
	world.WaitTicks(40);
	CHECK(world.Nearest() == 81);
	CHECK(world.JournalTried() == 1);
	CHECK(world.Told("blunt"));
	world.WaitTicks(60);
	world.Attack();
	world.WaitTicks(60);
	CHECK(world.Nearest() == 72);
	CHECK(world.JournalTried() == 1);
	world.Equip(ItemKind::ShortSword);
	world.WaitTicks(5);
	world.Attack();
	world.WaitTicks(60);
	CHECK(world.Nearest() == 47);
	CHECK(world.JournalTried() == 2);
	world.Equip(ItemKind::Spear);
	world.WaitTicks(5);
	world.Attack();
	world.WaitTicks(60);
	CHECK(world.Nearest() == 11);
	CHECK(world.JournalTried() == 3);
	world.SaveAndLoad();
	CHECK(world.JournalTried() == 3);
}

TEST_CASE("the amulet of venom poisons on a weapon hit that does not kill; the poison's kill is the player's") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_giant_rat"));
	world.Give(ItemKind::VenomNormal);
	world.Wear(ItemKind::VenomGrand);
	CHECK(world.items.Worn() == ItemKind::VenomGrand);
	REQUIRE(world.Walk(1.f));
	world.Wait(8000);
	CHECK(world.Nearest() == 60);
	CHECK(world.JournalTried() == 0);
	// club hits (10 HP each) until the venom takes (grand: 30%, strong)
	int hits = 0;
	while (world.dungeon.NearestMonsterPoison() == 0 && world.Nearest() > 10 && hits < 5) {
		world.Attack();
		world.WaitTicks(60);
		hits++;
	}
	REQUIRE(world.dungeon.NearestMonsterPoison() == 4); // strong
	CHECK(world.Nearest() == 60 - 10 * hits);
	CHECK(world.JournalTried() == 2); // blunt, and the poison
	world.Wait(20000); // strong on 60 HP: the floor, 3 HP a second
	CHECK(world.Nearest() == 0);
	CHECK(world.Xp() > 0);
}

TEST_CASE("health bars stay hidden until a monster acts on the player") {
	SimWorld world; // tests/levels/idle_monsters: a roosting bat, a plant, a scarab out of reach
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/idle_monsters"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.6f));
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.Walk(0.25f));
	world.WaitTicks(6);
	CHECK(world.dungeon.MonsterBarsShown() == 1); // the bat swoops
	REQUIRE(world.Walk(7.4f));
	world.Wait(3000);
	CHECK(world.dungeon.MonsterBarsShown() == 2); // the plant bites; never the scarab
}

TEST_CASE("the plant's bite poisons weakly on a share of bites; its sap on a share of melee hits only") {
	const MonsterKind* plant = monsterKind(MonsterPlant);
	CHECK(plant->poison == PoisonTier::Weak);
	CHECK(plant->poisonChancePercent == 30);
	REQUIRE(plant->touch.has_value());
	CHECK(plant->touch->chancePercent == 20);
	CHECK(monsterKind(MonsterScorpion)->poisonChancePercent == 100);
	CHECK_FALSE(monsterKind(MonsterScorpion)->touch.has_value());

	SimWorld world; // tests/levels/idle_monsters
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/idle_monsters"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.6f));
	REQUIRE(world.Walk(0.25f));
	REQUIRE(world.Walk(7.4f));
	world.Wait(20000); // some 25 bites
	CHECK(world.player.stats.poison.Mask() == 1);
}

TEST_CASE("the Anubis guard walks 20% faster than the player, the Anubis boss a little faster still") {
	CHECK(monsterKind(MonsterAnubisBoss)->speed > monsterKind(MonsterAnubis)->speed);
	// On the floor: tests/levels/reckless_anubis, the guard coming at the player standing still.
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_anubis"));
	world.WaitTicks(30);
	const Monster* anubis = world.dungeon.NearestMonster();
	REQUIRE(anubis != nullptr);
	world.WaitTicks(5);
	const float from = anubis->CentreX();
	world.Wait(1000);
	CHECK(from - anubis->CentreX() == doctest::Approx(1.2f * WALK_SPEED).epsilon(0.05)); // tiles in a second
}

TEST_CASE("fire arrows from a brazier: the mummy takes double from fire and burns on; the journal learns it") {
	SimWorld world; // tests/levels/fire: the mummy in its coffin at col 13
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/fire"));
	CHECK(resistedDamage(10, {0, 0, 0, 100}, monsterKind(MonsterMummy)->resist) == 20);
	world.Give(ItemKind::SelfBow);
	world.Equip(ItemKind::SelfBow);
	REQUIRE(world.WalkTo(12.f)); // it wakes and climbs out
	REQUIRE(world.WalkTo(9.5f)); // back to the brazier
	world.Interact(); // no brazier yet: nothing
	CHECK(world.player.fireArrows == 0);
	REQUIRE(world.dungeon.PlaceDecor(9, static_cast<int>(world.Y()), DECOR_BRAZIER));
	world.Interact();
	CHECK(world.player.fireArrows == FIRE_ARROW_SHOTS);
	REQUIRE(world.WalkTo(10.3f)); // in the bow's reach, facing it
	world.Wait(1000);
	REQUIRE(world.Nearest() == 150);
	CHECK_FALSE(world.dungeon.NearestMonsterBurns());
	world.Attack();
	world.WaitTicks(40);
	world.WaitTicks(40);
	// The bow's 12: 40% pierce (tough, 25%) and 60% fire (weak, 200%): 12 * 1.3 = 16.
	CHECK(world.Nearest() == 134);
	CHECK(world.dungeon.NearestMonsterBurns());
	CHECK(world.player.fireArrows == FIRE_ARROW_SHOTS - 1);
	CHECK(world.Told("Mummy catches fire"));
	CHECK(world.Told("weak to fire"));
	CHECK(world.Creature(MonsterMummy)->Tried(DamageType::Fire));
	// It burns a fifth of its fire share (14.4) a second for 3 s: 8.
	world.Wait(3500);
	CHECK_FALSE(world.dungeon.NearestMonsterBurns());
	CHECK(world.Nearest() == 126);
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::Fire;
	}));
}

TEST_CASE("the player burns through armour; half water puts the fire out") {
	SimWorld world; // tests/levels/fire: a pool of half water at cols 4-6
	REQUIRE(world.Load("tests/levels/fire"));
	world.player.stats.AddArmor(5);
	const int hp = world.Hp();
	world.player.TakeHit(10, {0, 0, 0, 100}, world.events);
	CHECK(world.Hp() == hp - 10);
	CHECK(world.player.burning.Any());
	world.Wait(3500);
	CHECK(world.Hp() == hp - 16);
	CHECK_FALSE(world.player.burning.Any());
	REQUIRE(world.WalkTo(5.5f));
	world.Wait(500);
	const int wet = world.Hp();
	world.player.TakeHit(10, {0, 0, 0, 100}, world.events);
	world.Wait(3500);
	CHECK(world.Hp() == wet - 10);
	CHECK_FALSE(world.player.burning.Any());
	CHECK(world.Told("The water puts the fire out"));
}
