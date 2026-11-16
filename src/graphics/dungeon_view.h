#ifndef DUNGEON_VIEW_H
#define DUNGEON_VIEW_H

// The dungeon's drawing (plan dungeon-view-split): the level's cells, the monsters, the mechanisms and the effects,
// over the GL-free Dungeon (src/sim/), and the state only the drawing keeps. A friend of Dungeon: the draw members read
// its state as they did when they were its own (dungeon_render*.cpp). GameState owns one, as it owns the PlayerView.
#include "../sim/dungeon.h"
#include <array>
#include <functional>
#include <vector>

class Item;
struct Assets;

class DungeonView {
  public:
	DungeonView(const Dungeon& dungeon, Assets& assets) : d(dungeon), assets(assets) {}
	void Draw(const HitboxView* hitboxes = nullptr); // hitboxes: the debug view, nullptr when off
	// The water over everything in it (the player too), see-through: after the player, in Draw's frame.
	void DrawWater();

  private:
	const Dungeon& d;
	Assets& assets; // the models and textures
	// The red contours (plan heka-surplus): queueContour keeps one with the modelview matrix of now, for drawContours
	// after the opaque scene (dungeon_render_outlines.cpp): the rim round what `draw` shows with the given texture
	// (the plain white one).
	struct Contour {
		std::array<float, 16> modelview;
		float strength;
		std::function<void(int texture)> draw;
	};
	std::vector<Contour> contours; // this frame's, drawn and cleared by drawContours
	struct ItemPartDraw {
		std::array<float, 16> modelview; // the item's frame, as Item::Draw had it
		Item* item;
		float rotA, scale;
	};
	std::vector<ItemPartDraw> itemParts; // this frame's, from DrawTreasureTile
	CellRect drawnCells; // the cells the last Draw drew, for DrawWater
	// A deep water cell's water inside its rock slabs, in world units from the cell's corner (WATER_SLAB).
	struct WaterBox {
		float left, right, bottom;
	};
	// The animations by the game clock: the portal texture's scroll, the riddle mark's yaw, the treasure item's spin
	// (degrees, one a tick).
	[[nodiscard]] static float portalScroll();
	[[nodiscard]] static float riddleMarkYaw();
	[[nodiscard]] static float treasureSpin();
	void drawChestGas(); // after the opaque scene, drawSummonEffects' frame
	void queueContour(float strength, std::function<void(int texture)> draw);
	void drawContours();
	void DrawMonsters(const CellRect& drawn); // at their actual position, not their spawn tile
	void drawDecoys(const CellRect& drawn); // with the frame origin of DrawMonsters
	void drawHitboxes(const HitboxView& weapon);
	void DrawTreasureTile(int i, int j); // the container, open or not, and the item once it shows
	void drawContainerEffects(); // the dust of a jar breaking, after the opaque scene
	// The see-through second parts of the items on the chests (a ring's gem, Item::DrawPart), after the opaque scene,
	// where DrawTreasureTile left them.
	void drawItemParts();
	void DrawTrapTile(bool isDeathTrap);
	void drawCeilingTrap(int i, int j); // CeilingSpikes, CeilingDeath (plan ceiling-spikes)
	void drawDecorTile(int i, int j);
	void drawDecalTile(int i, int j);
	void drawTorchTile(int i, int j);
	void drawLadderTile(int i, int j);
	void addLights(const CellRect& drawn);
	void drawFires(const CellRect& drawn);
	void drawCellSurfaces(int i, int j); // the rock face of a solid cell, the walls, floor and ceiling of an open one
	void drawShapeBlocks(int i, int j, float basinFloor); // a shaped cell's solid sub-cells (plan subcell-physics)
	void pushLevelFrame() const; // Draw's frame: the view's first cell at the origin, scrolled with the player
	void drawWaterCell(int i, int j, float x, float y);
	[[nodiscard]] bool deepAt(int col, int row) const;
	[[nodiscard]] WaterBox deepWaterBox(int col, int row) const;
	void drawRockStrip(int i, int j, float x0, float x1, float y0, float y1) const;
	void drawKeyTile(int i, int j);
	void drawPageTile(int i, int j);
	void drawGateTile(int i, int j);
	void drawLeverTile(int i, int j);
	// At the cell origin: the columns face the camera, plasma between them; a name gate's pulses (nameGateAlpha).
	void drawTeleporterTile(int i, int j);
	// A ring platform (plan ring-transporter): the plate in the floor, the housing in the ceiling, the rings while they
	// carry the player; a sealed one's plate breathes like a name gate's plasma.
	void drawRingPlatformTile(int i, int j);
	// The Stargate's inner track at its turn and its lit chevrons (plan stargate-chamber), in its prop's frame.
	void drawStargate(int i, int j);
	void addStargateLights(const CellRect& drawn); // the chevrons' glow on the walls round it
	// The plasma's opacity in a teleporter's cell: 1, but for a sealed name gate's breath and the shimmer of one just
	// opened.
	[[nodiscard]] float nameGateAlpha(int cell) const;
	void drawTileContent(int i, int j); // the fixture on the back wall, then what stands in the cell, by tile type
	void drawFixture(int i, int j);
	void drawAnkhTile();
	void drawDoorTile(int i, int j); // a sphinx gate: entrance, exit, riddle or empty
	// The scrolling plasma of the gates, over the quad v (its corners take the texture's (0, 0), (0, 1), (1, 1), (1,
	// 0)).
	void drawPortal(const float normal[3], const float v[4][3], bool red = false, float alpha = 1.f) const;
	void drawRockFallTile(int i, int j);
	void drawMechanismEffects(); // dust, after the opaque scene
	void drawDartPlateTile(int i, int j); // the plate and the dart holes above it
	void drawPressurePlateTile(int i, int j);
	void drawStoneTile(int i, int j);
	void drawInscriptionTile(int i, int j); // its atlas cell on the back wall
	void drawDarts(); // with the frame origin of DrawMonsters
	void drawWallSpikesTile(int i, int j); // the slotted panel and the spikes, as far out as the cycle has them
	void
	drawPoisonDrips(const CellRect& drawn); // the stain, the lump, the puddle and the falling venom, after the opaque
	void addPoisonDripLights(const CellRect& drawn); // a faint green glow off each puddle
	void drawFireVentTile(); // the vent low in the back wall
	void drawFireJets(const CellRect& drawn); // the jets and the telegraph's glow, after the opaque tiles
	void addFireVentLights(const CellRect& drawn); // the jets light the corridor, the telegraph glows
	void drawMirrorTile(int i, int j); // the stand, the mirror and the pointer showing its way
	void drawSunDiscTile(int i, int j);
	void drawSunBeams(); // after the opaque scene, drawSummonEffects' frame
	void addSunLights(const CellRect& drawn);
	void drawWebs(); // the webs, the burning ones in flames; drawSummonEffects' frame
	void drawSummonEffects(); // sand and dust where minions came out, the burrowers' holes, after the opaque scene
	// Sand on the floor (dungeon_render_sand.cpp, plan desert-chapter).
	[[nodiscard]] float driftHeight(int i, int j, float u, float v) const; // world units over the floor
	void drawSandDrift(int i, int j); // in drawCellSurfaces' frame
	void drawSandEffects(); // the steps' dust, a worm's ripple, the sand falls; drawSummonEffects' frame
	void drawGusts(); // the sand blown along the sandstorm halls (plan sandstorm-halls); drawSandEffects' frame
	// A sandstorm hall's shelter (plan sandstorm-halls): the side of cell (i, j) its pillar stands on, against the
	// back wall, -1 left / +1 right: the side a gust blows in from (a gust beside it blowing this way), or the side of
	// the pillar in the open cell below in a tall hall; 0: none.
	[[nodiscard]] int windbreakSide(int i, int j) const;
	// The pillar in drawCellSurfaces' frame, the wall texture bound: a base on the floor, a capital under the ceiling.
	static void drawWindbreak(int side, bool floor, bool ceiling);
	void drawRipple(float x, float y, float z, float size, int now) const; // size: 1 a worm's
	// A shaft of daylight falls through the cracked sandstone ceiling of (i, j) (CEILING_SANDSTONE_CRACKED, picked by
	// scatterSurfaces).
	[[nodiscard]] bool lightShaft(int i, int j) const;
	void drawHole(float x, float y, float z, float fade) const;
	void drawBlazes(); // drawSummonEffects' frame
	void addBlazeLights();
	void addVenomLights();
	void addFireLights(); // the fire arrows' and the burning monsters' (plan fire-damage)
	void drawBurning(); // flames on the burning monsters and the fire arrows; drawSummonEffects' frame
	void drawPowerSpark(); // drawSummonEffects' frame
	void drawStunStars(); // stars over each stunned monster's head (plan skill-stun); drawSummonEffects' frame
	void drawShots(); // with the frame origin of DrawMonsters
	void drawBalls(); // the balls rolling and in the windups; with the frame origin of DrawMonsters
	void drawBallEffects(); // Khepri's flames, the crumbs of the broken ones; drawSummonEffects' frame
	void addBallLights(); // the burning balls'; same frame as addLights
	void drawMissiles(); // with the frame origin of DrawMonsters
};

#endif
