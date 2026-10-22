#include "journal_text.h"
#include "monster_kinds.h"
#include <algorithm>
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
		if (t.id == MonsterBaboon)
			return "Screeches, lifts a burning coal over its head and hurls it. A jump clears it; water puts me out.";
		return t.ranged && t.ranged->burns() ? "Hurls a bolt of fire from afar. A jump clears it."
											 : "Spits venom from afar. A jump clears it.";
	case CreatureMove::Burrow:
		if (t.sandBurrow)
			return "In sand it swims under the surface and comes up under my feet. Watch the ripples.";
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
	case CreatureMove::Roar:
		return "Stunned against the wall, he gets up roaring a storm of sand. Do not stand next to him then.";
	case CreatureMove::Kite:
		return "Keeps its distance: backs off when I come at it, throws from afar. Corner it, or run it down.";
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
	{"Sand", "Sand. Underground. Somebody left a door open, three thousand years ago. I wade through the drifts at "
			 "three quarters of my pace, and running in it is out of the question. Jumping still works."},
	{"Quicksand", "The sand swallowed my boots, then my shins. Standing still makes it worse: the longer I stay, the "
				  "slower I get, until I am stuck and it squeezes. A jump gets me out, if I have the breath for it. "
				  "Up close it swirls, slowly. Look before you step."},
	{"Sun mirrors", "The priests knew how to bring the sun underground. So did a certain archaeologist with a staff. "
					"A shaft of daylight runs straight until rock stops it; a bronze mirror sends it on the way its "
					"pointer shows, and I can turn one a quarter at a time. A sun disc, an Aten, that catches the "
					"light opens the gates of its colour for good. The dead do not like the sun: it sets them alight."},
	{"Trapped chests", "Some chests bite back. Darts from the lock, a puff of green gas, a curse on the lid, deeper "
					   "down a nest of scarabs. Up close a trapped one looks darker, as if it knew. Half of them were "
					   "robbed long ago and only the trap is left; the rest hold a little more than a plain chest. The "
					   "Eye of Horus sees most traps before they go off."},
	{"Tomb curses", "The lid said 'cursed be he who opens this'. I opened it. The lid was right. The dead write their "
					"threats on whatever they leave behind: lids, jars, the little servant figures. Not every one "
					"means it, and the curse runs out, as any curse."},
	// The hour notes (plan campaign-texts): one per zone, in ZoneId order.
	{"The First Hour", "The gate of the tomb. Ra's boat enters the Duat and the first gate closes behind it. It closed "
					   "behind me too, and I did not even have a boat. The floors have teeth. Mind the floors."},
	{"The Second Hour", "Bats, and a smell of old leather. The wall says the second hour is the water of the blessed, "
						"where the grain grows. Nothing grows here except my doubts. And the bats."},
	{"The Third Hour", "The dark thickens, and the walls are hung with webs. The third hour is the hour of the judging "
					   "of Osiris. I am being judged by spiders. I keep a torch close."},
	{"The Fourth Hour", "The priests' quarter. Ra's boat has to be towed over sand here, says the wall, and the "
						"priests wrote a great deal on it. I can read it at last. I almost wish I could not."},
	{"The Fifth Hour", "The cavern of Sokar's secret, the wall says, and in my private chapter: the cavern of Serqet's "
					   "scorpions. I do not like her sense of humour. I count my antidotes."},
	{"The Sixth Hour", "The midnight of the journey: Ra and Osiris meet in the dark, and the serpents come to watch. "
					   "The wall says: the dead are made new here. I will settle for staying the same."},
	{"The Seventh Hour", "Apep waits here, says the wall. I wish the wall were wrong. The seventh hour is the hour of "
						 "the great serpent and of water: the boat is hauled through the cisterns. So am I."},
	{"The Eighth Hour", "Sand again, red and endless. Seth stands in the prow of the boat and drives the serpent back "
						"with his spear. The wall says the dead here are clothed. I would settle for a hat."},
	{"The Ninth Hour", "The land of Sokar, where the ground gives way and the worms sleep. The ninth hour feeds the "
					   "dead with grain, the wall says. I would be glad of a sandwich. I would be gladder of a floor."},
	{"The Tenth Hour", "A lake of fire, and baboons on its shore. In the tenth hour the lake is cooled for the boat of "
					   "Ra, so there must be water somewhere. I look for it first. Everything else second."},
	{"The Eleventh Hour", "The necropolis. Every trap I have met, in one place, and the road to the last gate. The "
						  "eleventh hour is the one before the dawn. The dawn is not what I would call close."},
	{"The Twelfth Hour", "The hall of the scales, the last hour. Ra is born again at its far end, says the wall. I "
						 "have a feeling that something here will weigh my heart. I did bring it. Let me be light."},
}};
} // namespace

const FieldNoteText& fieldNoteText(FieldNote note) { return FIELD_NOTES[static_cast<size_t>(note)]; }

namespace {
// The prologue: how the archaeologist came here. The statue is Anubis (the boss of lvl35), the ankh the one won there;
// the club is the starting weapon (items.cpp).
constexpr std::array<DiaryText, DIARY_PAGE_COUNT> DIARY = {{
	{"Saqqara, the third week",
	 {"The dig gave us nothing for nineteen days. On the twentieth, under the sand of the east trench, a statue no "
	  "catalogue knows: a jackal-headed man, life-sized, an ankh in his hand, and no name on the base. The crew would "
	  "not go near it. I am a scientist. I touched the ankh.",
	  "Light. Then cold stone and darkness. No sand, no sky, no crew. One torch burns on the wall, as if someone "
	  "expected me.",
	  nullptr}},
	{"Later",
	 {"At my feet lay an old acacia club. Not a trowel, but it will do: something scratches in the dark down the "
	  "hall. The corridor slopes down. There is no way back that I can see, so down it is.",
	  "The first signs I can read speak of one thing only: the ankh, life without end. Someone built this maze to keep "
	  "it. Or to keep someone in. If there is a way out, it is at the bottom, with the ankh.",
	  "I will keep notes. Someone should know what happened here."}},
}};
} // namespace

const DiaryText& diaryText(DiaryPage page) { return DIARY[static_cast<size_t>(page)]; }

namespace {
// The archaeologist who came before, D. J. (a nod to Daniel Jackson): a linguist, confident at first, then doubts; the
// last page warns before the fork. Each page lies a level or so before what it hints at (content/levels, Page tiles).
constexpr std::array<ExpeditionPageText, EXPEDITION_PAGE_COUNT> EXPEDITION_PAGES = {{
	{"Day 1",
	 "Touched the jackal statue. Should not have touched the jackal statue. No crew, no sky. I still have my notebook, "
	 "two pencils and my glasses. If anyone finds this: I am D. J., a linguist, not a soldier."},
	{"Day 2", "The beetles grow bigger the deeper I go. A blade skids off the shell; a point slips between the plates. "
			  "Something very large clicks down below. I named it Ramses. It does not like the name."},
	{"Day 3", "A chest bit me today. Not a figure of speech. Wood cracks under a club; my knife only stuck in the lid. "
			  "New rule: "
			  "knock before opening."},
	{"Day 5",
	 "Ramses is dead. I did not enjoy it. Behind him a key and a way down, never a way up. Every door here opens one "
	 "way: deeper. Someone planned this."},
	{"Day 6",
	 "Water! I could have kissed it, until the log in it opened its eyes. The crocodiles swim much faster than I "
	 "wade. Stay out of the pools when you can, or cross them fast."},
	{"Day 8",
	 "Bats. A club swats them, an arrow goes through the wing. Something up in the roost drinks blood and heals with "
	 "every bite. Do not let it bite. Thin out its friends first."},
	{"Day 9",
	 "Webs across the halls, thick as sailcloth, and things that spin them: eight legs, fast, and they hate fire. "
	 "A torch in the hand keeps them at a distance. Mine went out once. I will not describe that hour."},
	{"Day 11",
	 "The dead do not stay down here. Linen tears under a blade and burns like paper: dip the arrows in a brazier and "
	 "they go up. Jack would have brought a flamethrower. Who is Jack?"},
	{"Day 12",
	 "A hall with two shafts side by side, and a priest's warning on the wall. I took the short one; the long one "
	 "had water in it and I am a bad swimmer. Both come out in the same hall. There is no way back up to the other."},
	{"Day 13",
	 "I can read most of the walls now. The priests write of a ring of stone with seven marks, a road between worlds. "
	 "If it is down here, it is hidden behind a riddle. My last pencil to see it."},
	{"Day 15",
	 "Scorpions everywhere, and eggs. Something lays them. Cut or burn the clusters before her brood hatches, and "
	 "carry an antidote. Two. I sneezed for an hour: allergic to something here. Everything."},
	{"Day 16",
	 "The roof weeps green in the nest halls. One drop burns, two put me on my knees. Wait for the gap between the "
	 "drops, then run. The antidote tastes of boiled socks. I have drunk four."},
	{"Day 18",
	 "The serpent temple. The cobras rear up when I come close and spit from afar: jump the spit. The great one below "
	 "dives into the floor and comes up behind you. Turn around. Always turn around."},
	{"Day 20",
	 "My pencil is half its length. I have stopped believing in a way up. I still believe in a way out. Not the same "
	 "thing, I know. It is what I have."},
	{"Day 23",
	 "Darts fly from the wall where the floor clicks: jump the plate. Lost my glasses for an hour and found them on a "
	 "mummy. It was wearing them. We had words."},
	{"Day 24",
	 "The cisterns have a deep end. Down there I swim, and swimming tires me twice as fast as walking. Rest on dry "
	 "stone, drink the blessing of Hapi before the water, not in it. Crocodiles in every pool, hippos in the big "
	 "ones."},
	{"Day 26",
	 "Sand. Underground! It drags at the feet, and where it swirls it swallows: jump out, do not wait. The spiders "
	 "here hate the light. I sleep by the torches. Well, I lie by them."},
	{"Day 29",
	 "Sunlight through a crack, the first in weeks. Bronze mirrors turn it onto the sun discs that open the doors, and "
	 "the mummies burn in it. The red one in the pool below charges, then roars. Stand aside."},
	{"Day 30",
	 "Worms. Not the kind in the garden. They hear a footstep on sand and come up under it; the spiders hunt in "
	 "the dark between. A fork ahead: one way over loose drifts, one along the rock. Rock, rock, rock."},
	{"Day 31",
	 "Some doors want a name, not a key. The king's name is carved in a cartouche on the same level: find it, copy "
	 "it, speak it. Names had power here. Mine does not seem to."},
	{"Day 35",
	 "The chests are darker down here, and some of them follow you. One bit me and my mind went cloudy for minutes. "
	 "Dark wood: knock with a club first. Abydos is not on this map."},
	{"Day 38",
	 "A lake of fire, and baboons on its shore who throw coals from afar. Corner them or run them down; never "
	 "stand in the open. The water is the one cool place here. A second fork ahead: the right is shorter. It "
	 "always is."},
	{"Day 40", "The necropolis. Loose ceilings everywhere: when the stone rumbles, keep walking. The jackal from the "
			   "statue is on "
			   "every wall. He waits at the bottom. I think he has always been waiting."},
	{"The last page",
	 "If you read this, take the left gate. I took the right. Behind it, the tomb of the one who came before me, and "
	 "worse. The ankh is real. Tell them I was right. D. J."},
}};
} // namespace

const ExpeditionPageText& expeditionPageText(int number) {
	return EXPEDITION_PAGES[static_cast<size_t>(std::clamp(number, 1, EXPEDITION_PAGE_COUNT) - 1)];
}
