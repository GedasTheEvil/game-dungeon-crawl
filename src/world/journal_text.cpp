#include "journal_text.h"
#include "monster_kinds.h"
#include <array>

namespace {

// The curse its hits lay, the first one listed (melee first). The mummy priest's are too many to name.
const char* curseMoveNote(const MonsterKind& t) {
	if (t.id == MonsterMummyPriest)
		return "Curses with a touch of his staff, and speaks curses from afar.";
	const CurseEntry& e = t.meleeCurses[0].percent > 0 ? t.meleeCurses[0] : t.rangedCurses[0];
	switch (e.kind) {
	case Curse::Weakness:
		return "Its blows curse: my arm goes weak.";
	case Curse::Clumsiness:
		return "Its bite curses: my blows get weaker.";
	case Curse::Vulnerability:
		return "Its blows curse: my armour turns to papyrus.";
	case Curse::Fatigue:
		return "Its bite curses: my strength drains away.";
	case Curse::Disease:
		return "Its bite curses: a sickness eats my health.";
	case Curse::Slowness:
		return "Its bite curses: my legs turn to stone.";
	case Curse::Palsy:
		return "Its bolt curses: my hands shake too much to aim.";
	case Curse::Insanity:
		return "Its bite curses: my mind clouds over.";
	}
	return "Its touch curses.";
}

} // namespace

const char* creatureMoveNote(CreatureMove move, const MonsterKind& t) {
	switch (move) {
	case CreatureMove::Leap:
		return "Leaps over pits and traps.";
	case CreatureMove::Swoop:
		return "Hangs from the ceiling, swoops down to bite.";
	case CreatureMove::Ambush:
		return "Lies in wait as a treasure chest!";
	case CreatureMove::Rise:
		return "Lies in its coffin, climbs out when I come near.";
	case CreatureMove::Summon:
		switch (t.boss.summon) {
		case Summon::DigOut:
			return "Calls its young up out of the sand.";
		case Summon::Drop:
			return "Calls its kind down from the ceiling.";
		case Summon::Coffin:
			return "The dead climb out of their coffins at its call.";
		case Summon::Hatch:
			return "Her young hatch from the egg clusters around her.";
		case Summon::Wall:
			if (t.boss.smallMinion != 0)
				return "His statues and his servants step out of the stone at his call.";
			return "His statues step out of the stone at his call.";
		}
		break;
	case CreatureMove::Heal:
		return "Heals as it bites.";
	case CreatureMove::Surface:
		return "Lies under the water, only its eyes show.";
	case CreatureMove::Poison:
		return "Its poison keeps burning long after the bite.";
	case CreatureMove::Rear:
		return "Lies coiled, rears up when I come near.";
	case CreatureMove::AttackRanged:
		if (t.id == MonsterMummyPriest)
			return "Speaks curses from afar, a bolt of purple light. A jump clears it.";
		return t.ranged && t.ranged->burns() ? "Hurls a bolt of fire from afar. A jump clears it."
											 : "Spits venom from afar. A jump clears it.";
	case CreatureMove::Burrow:
		return "Dives into the sand and comes up behind me.";
	case CreatureMove::Charge:
		return "Charges along the hall. A wall stuns it.";
	case CreatureMove::Climb:
		return "Follows me up and down the ladders.";
	case CreatureMove::Curse:
		return curseMoveNote(t);
	case CreatureMove::Bloom:
		return "Floats among the lotus flowers, rears up when I come near.";
	case CreatureMove::Sedate:
		return "Its bite makes me drowsy, too tired to run or jump.";
	case CreatureMove::ShunLight:
		return "Will not come into the torchlight. It waits in the dark until I leave it.";
	case CreatureMove::Grab:
		if (t.hold && t.hold->sting)
			return "Two claws to hold you, one tail to finish. Hit the claws.";
		return "In the water its jaws hold me fast and it rolls. When they gape, step back.";
	case CreatureMove::StepOff:
		return "Stands among the other figures, still as they are, and steps off its place when I come near.";
	}
	return "";
}

namespace {
// By FieldNote. The rules as the game has them (PlayerStats, ItemBag, the mechanisms); keep them in step.
constexpr std::array<FieldNoteText, FIELD_NOTE_COUNT> FIELD_NOTES = {{
	{"Hit points", "Every bite, blow and trap costs me health: the red bar in the corner. When it runs out, the "
				   "expedition is over. A new level fills it up."},
	{"Experience", "Every creature I kill and every riddle I answer teaches me something. With enough of it I reach "
				   "a new level: more health and more stamina, both filled up at once, and any poison gone. Every "
				   "fifth level my skin gets tougher, every eighth my arm stronger."},
	{"Stamina", "Jumping and running (Shift) tire me out: the yellow bar. Each jump uses up some of it, and running "
				"drains it all the time. When I walk or stand, it comes back by itself."},
	{"Keys and gates", "A key opens every gate of its colour on this level. Some gates answer to a lever of their "
					   "colour instead. The gate of a boss's lair stays shut until the boss is dead."},
	{"Poison", "Some stings and bites poison me: the health bar turns green and keeps draining after the fight, "
			   "armour or not, and it can kill me. The same poison again only starts over; a stronger or weaker one "
			   "runs alongside it. A new level clears all poison."},
	{"Weapons", "Copies of my weapons are in chests, and now and then a creature leaves one; with enough copies "
				"a weapon can be upgraded in the inventory (U). The old club gains the most from it. 1 takes up the "
				"next hand weapon, 2 the next one to shoot or throw. Every creature takes each kind of blow "
				"differently: I note it down."},
	{"Smash", "A club smashes, and so do the mace, the sling's stones and the throwing stick. They hit hard, but "
			  "they do not cut."},
	{"Cut", "A blade cuts: the bronze sword, the khopesh (a curved sword), the epsilon axe. Long cuts, quick to "
			"follow one another."},
	{"Stab", "A point stabs: the flint knife, the spear, the javelin, the duckbill axe, every arrow. It finds the "
			 "gaps between scales and plates."},
	{"Health potions", "Bitter herbs from the marshes heal my wounds, the priests' brew twice as much. When I am "
					   "in a hurry, H drinks the one that fits best."},
	{"Stamina potions", "Date wine, or the red beer of Hathor: a stamina potion gives me my stamina back. When I "
						"am in a hurry, 0 drinks the one that fits best."},
	{"Blood of Montu", "It tingles. Whatever is in it, my arm hits harder after I drink it, forever."},
	{"Granite of Ptah",
	 "Ptah's granite: my skin becomes hard as a temple wall, and every blow hurts a little less, forever."},
	{"Breath of Osiris", "The breath of Osiris: more health forever, and all my wounds close at once."},
	{"Milk of Renenutet",
	 "The milk of Renenutet cures every poison in me. = drinks one, but only while I am poisoned."},
	{"Dart traps", "A slab with a cobra on it sank under my foot, and darts shot out of three holes in the wall "
				   "above it, tipped with venom. A rat is too light to press it, a mummy is not, and the darts hit "
				   "whoever is in their way. A jump over the slab sets nothing off; a jump at the right moment lets "
				   "them pass under me."},
	{"Resistance potions", "Cobra venom, a drop at a time. For two minutes a poisoned bite or sting often does "
						   "nothing to me: half the time after the lesser potion, nearly always after the greater "
						   "one. The greater potion also cures a poison already in me. A poison guard amulet adds "
						   "its share."},
	{"Heka", "The guardian's heka, the magic the gods hold, went into me. It tastes of river mud and old incense. "
			 "Each guardian gives one; on the Heka page (K) I spend it on a skill, forever. There are more skills "
			 "than guardians down here, so I choose with care."},
	{"Serket's favour", "Serket likes me. Her children still bite, but they hurt me less: every poison on me "
						"lasts half as long, with the same damage each second."},
	{"Sharp eye", "Horus lent me an eye. I will give it back. Probably. Meanwhile my bows, the sling, the javelin and "
				  "the throwing stick all reach a third farther: even a foe far away is in my sights."},
	{"Weakness", "A curse sits on my arm: half my might is gone, and the greater curse takes all of it and more. "
				 "Nothing I drink removes a curse. Two minutes, then it fades. Another curse of the same kind "
				 "restarts the two minutes."},
	{"Clumsiness", "Cursed with clumsy hands: every blow, shot and throw does a fifth less damage, and half as much "
				   "under the greater curse. My venom is not affected. Two minutes and it passes; no potion helps."},
	{"Vulnerability", "Cursed: my armour feels like papyrus. It stops only two fifths of what it did. "
					  "Two minutes, and nothing cures it. I should not get hit in that time."},
	{"Fatigue", "A curse drained me: my stamina fell to a fifth and does not come back by itself for two minutes. "
				"A stamina potion still works."},
	{"Disease", "A cursed sickness: my body holds less health, a third less, much less under the greater curse. "
				"It gives the health back when it passes, in two minutes. No antidote helps."},
	{"Slowness", "Cursed legs: I walk and climb at half speed, and I run hardly faster than I used to walk. Two "
				 "minutes. Traps made for a quick step are dangerous for me now."},
	{"Palsy", "My hands shake from a curse: no bow, sling or throw finds its mark, so I do not try. My melee weapons "
			  "still work. Two minutes."},
	{"Insanity", "My mind is clouded. The club stays in my hand, and I cannot use my map, journal, pack or potions. "
				 "Two minutes, if I live."},
	{"Fire",
	 "Fire sets what it hits on fire: it keeps burning for a few seconds after the hit. Armour does not stop it, "
	 "only a fire guard amulet does, and water puts it out. A brazier lights my next five arrows (E by "
	 "it). Mummies, it turns out, burn very well. Whatever shrugs off an arrow, a bat's thin wing, a mimic's dry wood, "
	 "a plant, a mummy, burns instead: fire arrows are my answer to it. Scorpions are used to the heat."},
	{"Favour of Hapi", "The water parts for me now. Not much, but enough to stop wading like a heron."},
	{"Blessing of Isis", "Isis has a knot for every curse. She ties fast."},
	{"Blessing of Hapi", "Hapi's jar. For two minutes the Nile and I are on good terms: I wade at full speed, sprint "
						 "and jump in half water as if it were a street. With his favour it lasts twice as long and "
						 "the water carries me."},
	{"Canopic jars", "Four jars, four sons of Horus: one each for the liver, the lungs, the stomach and the guts. "
					 "Somebody swapped the insides for treasure. I try not to wonder where the insides went."},
	{"Jumping a bite", "It snapped at my heels and caught only air. A small one bites low: jump as it opens its jaws "
					   "and the bite goes under me. Over a tall one a jump does not help."},
}};
} // namespace

const FieldNoteText& fieldNoteText(FieldNote note) { return FIELD_NOTES[static_cast<size_t>(note)]; }
