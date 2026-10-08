// Model viewer: loads a .md3 file with the game's AnimatedModel, plays its animation on a loop that restarts every
// [seconds], with a progress bar of the loop and a stats panel. Runs from the repo root.
//
// Usage: build/model-viewer <model.md3> [seconds] [options]
//   <model.md3>  required, path to a .md3 file (see tools/blender/md3.py)
//   [seconds]    optional, animation loop duration in seconds (default 5.0).
// Options:
//   --light flat|game|toon  start with this lighting (default flat; L cycles it)
//   --yaw <deg> --pitch <deg>  start turned this way (default 20, 0)
//   --texture <name>        start with this texture, e.g. rat_giant (default the first, see TextureCandidates)
//   --shot <dir>            screenshot mode: writes <dir>/<stem>_<frame>.png for the frames and exits, no HUD
//   --frames 0,4,8|all      the frames for --shot (default all)
//   --size <w>x<h>          the window size (default 800x600)
// Headless shots: xvfb-run -a -s "-screen 0 1280x1024x24" build/model-viewer <model.md3> --shot <dir> ...
//
// Keys: space cycles through the model's clips, T through its textures, L the lighting (flat, game, toon),
// + / - speed the loop up / slow it down. Drag with the left mouse button to turn the model.

#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <memory>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>

#include "../../src/graphics/animated_model.h"
#include "../../src/graphics/textures.h"
#include "../../src/graphics/lighting.h"
#include "../../src/graphics/ink.h"
#include "hud.h"
#include "../../src/graphics/font.h"
#include "../../src/world/items.h"
#include "../../src/ui/ui_draw.h"
#include "../../src/core/logger.h"
#include "../../src/core/timer.h"
#include "../../external/stb/stb_image_write.h"

namespace {

// AnimatedModel whose frame follows an external wall-clock ratio instead of Advance().
class LoopedAnimatedModel : public AnimatedModel {
  public:
	// VCount is protected in AnimatedModel. AnimatedModel::Show()/Compile()
	// both draw with glDrawArrays(GL_TRIANGLES, 0, VCount) -- a flat,
	// non-indexed triangle list -- so every 3 vertices are one triangle.
	int TriangleCount() const { return VCount / 3; }

	void SetFrame(int frame) { playback.frame = static_cast<float>(std::clamp(frame, 0, std::max(frameC - 1, 0))); }

	// Maps ratio in [0, 1) onto frame in [0, frameC).
	void SetProgress(float ratio) {
		if (frameC <= 1) {
			playback.frame = 0.0f;
			return;
		}
		if (ratio < 0.0f)
			ratio = 0.0f;
		else if (ratio > 1.0f)
			ratio = 1.0f;

		playback.frame = ratio * static_cast<float>(frameC);
		if (playback.frame >= static_cast<float>(frameC))
			playback.frame = static_cast<float>(frameC) - 1.0f;
	}
};

std::unique_ptr<LoopedAnimatedModel> gModel;
int gWinWidth = 800;
int gWinHeight = 600;
int gStartTicks = 0;
double gDurationSeconds = 5.0;
const double SPEED_STEP = 1.25; // + divides the loop duration by this, - multiplies it
const double MIN_DURATION_SECONDS = 0.2;
const double MAX_DURATION_SECONDS = 60.0;

float gYawDeg = 20.0f;	// matches the current fixed view exactly, so the
float gPitchDeg = 0.0f; // initial frame on launch is unchanged
bool gDragging = false;
int gLastMouseX = 0;
int gLastMouseY = 0;
const float DRAG_DEG_PER_PX = 0.4f; // empirical; tune by feel
const float MAX_PITCH_DEG = 89.0f;	// avoid flipping past vertical

// Stats panel: set by ApplyLoadedModel for every loaded model.
Font gStatsFont;
Font gHintFont;
std::string gStatModelName;
int gStatFrameCount = 0;
int gStatPolygonCount = 0;

// The texture object lives at file scope so ApplyLoadedModel can reuse it across both the initial load and every
// [space] / [T] switch; the sibling group is scanned once at startup and never rescanned.
Texture gTexture;
std::vector<std::string> gTexturePaths; // the model's texture candidates, see TextureCandidates
std::size_t gTextureIndex = 0;
std::vector<std::string> gSiblingModelPaths;
std::size_t gCurrentSiblingIndex = 0;

// Flat: texture only, as the old viewer. Game: the game's lighting (Lighting), the player's light in front of the
// model. Toon: game lighting in toon mode with the ink outlines (Ink), as F1 in the game.
enum class LightMode : std::uint8_t { Flat, Game, Toon };
constexpr const char* LIGHT_MODE_NAMES[] = {"flat", "game", "toon"};
LightMode gLightMode = LightMode::Flat;
// The player's light as a monster a tile away gets it: the game's radius over the figure's size (~19 world units),
// the model here being about 1 unit. Placed in front of the model's chest, on the camera's side.
constexpr Lighting::LightDef VIEWER_LIGHT = {Lighting::PLAYER.r, Lighting::PLAYER.g, Lighting::PLAYER.b, 5.f, 0.f};
constexpr float VIEWER_LIGHT_POS[3] = {0.35f, 0.6f, 1.0f}; // before the model's turn, in the camera's frame
constexpr double Z_NEAR = 0.1;
constexpr double Z_FAR = 100.0;

// Screenshot mode (--shot): the frames to write, then exit.
std::string gShotDir;
std::vector<int> gShotFrames; // empty: all

// Returns the filename with its directory stripped but extension kept,
// e.g. "models/monsters/anubis.md3" -> "anubis.md3". Distinct from FileStem()
// (which also strips the extension, for the window title's own use).
std::string Basename(const std::string& path) {
	std::size_t slash = path.find_last_of("/\\");
	return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

// "models/monsters/anubis.md3" -> "anubis".
std::string FileStem(const std::string& path) {
	std::size_t slash = path.find_last_of("/\\");
	std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
	std::size_t dot = base.find_last_of('.');
	if (dot == std::string::npos)
		return base;
	return base.substr(0, dot);
}

// The stem of the model whose clip this file is: the shortest cut of the stem at a '_' that names a .md3 in the same
// directory, or the whole stem. "anubis_att" -> "anubis", "egg_cluster_die" -> "egg_cluster",
// "decor_osiris" -> "decor_osiris" (there is no decor.md3), "ankh" -> "ankh".
std::string BaseStem(const std::string& modelPath) {
	const std::filesystem::path dir = std::filesystem::path(modelPath).parent_path();
	std::string stem = FileStem(modelPath);
	for (std::size_t cut = stem.find('_'); cut != std::string::npos; cut = stem.find('_', cut + 1)) {
		std::string prefix = stem.substr(0, cut);
		std::error_code ec;
		if (std::filesystem::is_regular_file(dir / (prefix + ".md3"), ec))
			return prefix;
	}
	return stem;
}

// Every *.md3 in modelPath's directory with the same base stem (its clips), sorted by filename.
// Always includes modelPath itself.
std::vector<std::string> ScanSiblingModels(const std::string& modelPath) {
	std::filesystem::path path(modelPath);
	std::filesystem::path dir = path.parent_path();
	if (dir.empty())
		dir = ".";
	const std::string baseStem = BaseStem(modelPath);
	const std::string loadedBasename = path.filename().string();
	std::vector<std::string> result;
	bool foundLoadedFile = false;

	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		if (!entry.is_regular_file())
			continue;
		const bool isLoadedFile = entry.path().filename().string() == loadedBasename;
		if (!isLoadedFile &&
			(entry.path().extension() != ".md3" || BaseStem((dir / entry.path().filename()).string()) != baseStem))
			continue;
		foundLoadedFile |= isLoadedFile;
		result.push_back((dir / entry.path().filename()).string());
	}
	if (ec)
		return {modelPath};
	if (!foundLoadedFile)
		result.push_back(modelPath);
	std::sort(result.begin(), result.end());
	return result;
}

// The textures that fit a model, in textures/<category>/ (<category>: the model's sub-directory under models/):
//   1. <stem>.png, the file's own (decorations, items, a clip with a texture of its own)
//   2. <base>.png, the texture its clips share (models/monsters/anubis_att.md3 -> textures/monsters/anubis.png)
//   3. <base>_*.png, the variants that reuse the model (anubis_boss, rat_giant, key_blue), sorted
//   4. a potion vessel (items/potion_flask): its potions' textures, in ItemKind order (PotionDef::texture)
// Empty when none exists.
std::vector<std::string> TextureCandidates(const std::string& modelPath) {
	const std::string category = std::filesystem::path(modelPath).parent_path().filename().string();
	const std::filesystem::path dir = std::filesystem::path("textures") / category;
	const std::string stem = FileStem(modelPath);
	const std::string baseStem = BaseStem(modelPath);
	std::vector<std::string> result;
	std::error_code ec;
	for (const std::string& name : {stem, baseStem}) {
		std::string path = (dir / (name + ".png")).string();
		if (std::filesystem::is_regular_file(path, ec) && std::find(result.begin(), result.end(), path) == result.end())
			result.push_back(path);
	}

	std::vector<std::string> variants;
	const std::string prefix = baseStem + "_";
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		const std::string name = entry.path().filename().string();
		std::string path = (dir / name).string();
		if (entry.is_regular_file() && entry.path().extension() == ".png" && name.rfind(prefix, 0) == 0 &&
			std::find(result.begin(), result.end(), path) == result.end())
			variants.push_back(path);
	}
	std::sort(variants.begin(), variants.end());
	result.insert(result.end(), variants.begin(), variants.end());

	if (category == "items")
		for (int i = WEAPON_KIND_COUNT; i < FIRST_AMULET; i++) {
			const PotionDef& potion = potionDef(itemAt(i));
			std::string path = (dir / (std::string(potion.texture) + ".png")).string();
			if (stem == potionModelDef(potion.model).model && std::filesystem::is_regular_file(path, ec))
				result.push_back(path);
		}
	return result;
}

// Loads gTexturePaths[gTextureIndex], else textures/null.png, else nothing (id 0). Every failure is logged as a
// warning, never fatal -- a missing/wrong texture must never prevent seeing the animation.
int LoadCurrentTexture() {
	if (gTextureIndex < gTexturePaths.size()) {
		const std::string& path = gTexturePaths[gTextureIndex];
		if (gTexture.LoadPNG(path.c_str())) {
			LOG_INFOF("modelviewer", "Texture: %s", path.c_str());
			return gTexture.ID();
		}
	} else {
		LOG_WARNINGF("modelviewer", "No texture found for %s, falling back to textures/null.png",
					 gStatModelName.c_str());
	}
	if (gTexture.LoadPNG("textures/null.png")) {
		return gTexture.ID();
	}

	LOG_WARNINGF("modelviewer", "%s", "textures/null.png fallback also failed, continuing untextured");
	return 0;
}

// gModel must already have a successful Load() by the time this runs --
// called once from main() for the initial model, and again from
// KeyPressed() on every [space] cycle. Resets the loop timer and stats;
// deliberately leaves gDurationSeconds/gYawDeg/gPitchDeg untouched.
// Keeps the picked texture when the new clip has it too (anubis_boss stays on through the clips).
void ApplyLoadedModel(const std::string& path) {
	const std::string previousTexture = gTextureIndex < gTexturePaths.size() ? gTexturePaths[gTextureIndex] : "";
	gStatModelName = Basename(path);
	gTexturePaths = TextureCandidates(path);
	auto kept = std::find(gTexturePaths.begin(), gTexturePaths.end(), previousTexture);
	gTextureIndex = kept == gTexturePaths.end() ? 0 : static_cast<std::size_t>(kept - gTexturePaths.begin());

	gModel->BindTexture(LoadCurrentTexture());
	gModel->Centrify();
	gModel->Compile();

	gStartTicks = GameClock::now();

	gStatFrameCount = gModel->FrameCount();
	gStatPolygonCount = gModel->TriangleCount();
}

// Where the loop is, in [0, 1).
double LoopRatio() {
	double elapsedSeconds = (GameClock::now() - gStartTicks) / 1000.0;
	return std::fmod(elapsedSeconds, gDurationSeconds) / gDurationSeconds;
}

// Changes the loop duration, clamped, and moves the loop start so the model keeps its pose.
void SetDuration(double seconds) {
	const double ratio = LoopRatio();
	gDurationSeconds = std::clamp(seconds, MIN_DURATION_SECONDS, MAX_DURATION_SECONDS);
	gStartTicks = GameClock::now() - static_cast<int>(ratio * gDurationSeconds * 1000.0);
}

// Stats panel at the top right, in the look of the game's status box (docs/ui.md): dark framed panel, gold text.
void DrawStatsPanel() {
	// ---- layout ----, on a canvas 100 high
	constexpr float MARGIN = 2.f;  // panel to the window edges
	constexpr float PAD = 3.f;	   // text to the frames
	constexpr float LINE_H = 4.6f; // baseline to baseline
	constexpr float HINT_H = 4.f;
	constexpr float GLYPH_HIGH = 3.f; // the body font's letters reach this far above the pen y

	char lines[6][160];
	std::snprintf(lines[0], sizeof lines[0], "%s", gStatModelName.c_str());
	std::snprintf(lines[1], sizeof lines[1], "Clip %zu / %zu", gCurrentSiblingIndex + 1, gSiblingModelPaths.size());
	std::snprintf(lines[2], sizeof lines[2], "Frames: %d   Polygons: %d", gStatFrameCount, gStatPolygonCount);
	std::snprintf(lines[3], sizeof lines[3], "Loop: %.2f s", gDurationSeconds);
	if (gTextureIndex < gTexturePaths.size())
		std::snprintf(lines[4], sizeof lines[4], "Texture %zu / %zu: %s", gTextureIndex + 1, gTexturePaths.size(),
					  Basename(gTexturePaths[gTextureIndex]).c_str());
	else
		std::snprintf(lines[4], sizeof lines[4], "Texture: none");
	std::snprintf(lines[5], sizeof lines[5], "Light: %s", LIGHT_MODE_NAMES[static_cast<int>(gLightMode)]);
	const char* hint = "space clip   T texture   L light   + / - speed   drag turn";

	float canvasW = ui::beginSquareCanvas(100.f, gWinWidth, gWinHeight);
	glLoadIdentity();
	float textW = gHintFont.TextWidth(hint);
	for (const auto& line : lines)
		textW = std::max(textW, gStatsFont.TextWidth(line));
	const float w = textW + 2 * PAD;
	const float h = 2 * PAD + GLYPH_HIGH + LINE_H * 5 + HINT_H;
	const ui::Rect box = {canvasW - MARGIN - w, 100.f - MARGIN - h, w, h};

	ui::beginShapes();
	ui::fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, ui::BLACK, ui::BLACK, 0.45f); // drop shadow
	ui::panel(box, 0.92f);

	ui::beginText();
	float y = box.y + box.h - PAD - GLYPH_HIGH;
	for (std::size_t i = 0; i < std::size(lines); i++) {
		ui::text(gStatsFont, box.x + PAD, y, lines[i], i == 0 ? ui::GOLD_BRIGHT : ui::GOLD);
		y -= LINE_H;
	}
	ui::text(gHintFont, box.x + PAD, y + LINE_H - HINT_H, hint, ui::GOLD_DIM);
}

void InitGL(int width, int height) {
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.15f, 0.15f, 0.2f, 0.0f);
	glClearDepth(1.0);
	glShadeModel(GL_SMOOTH);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(width) / static_cast<double>(height), Z_NEAR, Z_FAR);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void SetLightMode(LightMode mode) {
	gLightMode = mode;
	Ink::setToon(mode == LightMode::Toon);
}

// The model at its current frame, lit as gLightMode says.
void DrawScene() {
	Ink::begin(static_cast<float>(Z_NEAR), static_cast<float>(Z_FAR), gWinWidth, gWinHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Re-issue the 3D perspective projection every frame (mirrors
	// src/graphics/draw.cpp's Draw(), which also sets up gluPerspective from
	// scratch each call) since the 2D overlay pass below switches the
	// projection matrix to an orthographic one for the HUD bar.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(gWinWidth) / static_cast<double>(gWinHeight), Z_NEAR, Z_FAR);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Centrify() normalizes the model to roughly unit size with its base at
	// y=0 and centered on x/z, so a fixed camera a couple of units back and
	// half a unit down (to vertically center the ~1-unit-tall model) frames
	// it reasonably for any model. A static yaw gives a 3/4 view instead of
	// a flat front-on silhouette; nothing here animates the model itself.
	if (gShotDir.empty())
		glTranslatef(-0.35f, -0.5f, -2.2f);
	else
		glTranslatef(0.f, -0.5f, -2.f); // no HUD to make room for: centred, a bit closer
	const bool lit = gLightMode != LightMode::Flat;
	if (lit) {
		Lighting::begin();
		Lighting::add(VIEWER_LIGHT_POS[0], VIEWER_LIGHT_POS[1], VIEWER_LIGHT_POS[2], VIEWER_LIGHT, 0);
		Lighting::commit();
	}
	glRotatef(gYawDeg, 0.0f, 1.0f, 0.0f);
	glRotatef(gPitchDeg, 1.0f, 0.0f, 0.0f);

	if (gModel)
		gModel->Show();
	if (lit)
		Lighting::end();
	Ink::end();
}

// Screenshot mode: writes every asked frame of the model to gShotDir and exits.
void WriteShots() {
	std::error_code ec;
	std::filesystem::create_directories(gShotDir, ec);
	std::vector<int> frames = gShotFrames;
	if (frames.empty())
		for (int f = 0; f < gModel->FrameCount(); f++)
			frames.push_back(f);
	std::vector<unsigned char> pixels(static_cast<size_t>(gWinWidth) * static_cast<size_t>(gWinHeight) * 3);
	stbi_flip_vertically_on_write(1);
	int failures = 0;
	for (int f : frames) {
		gModel->SetFrame(f);
		DrawScene();
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, gWinWidth, gWinHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
		char file[512];
		std::snprintf(file, sizeof file, "%s/%s_%02d.png", gShotDir.c_str(), FileStem(gStatModelName).c_str(), f);
		if (stbi_write_png(file, gWinWidth, gWinHeight, 3, pixels.data(), gWinWidth * 3)) {
			std::printf("saved %s\n", file);
		} else {
			std::fprintf(stderr, "cannot write %s\n", file);
			failures++;
		}
	}
	std::fflush(stdout);
	std::exit(failures ? 1 : 0);
}

void Display() {
	if (!gShotDir.empty())
		WriteShots();

	double loopRatio = LoopRatio();
	if (gModel)
		gModel->SetProgress(static_cast<float>(loopRatio));

	DrawScene();

	// 2D loop-progress bar overlay, using the same
	// glOrtho(0, 100, 0, 100, -21, 21) 2D-overlay convention
	// src/graphics/draw.cpp's Draw() uses for the in-game HUD.
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// The model's texture is still bound from the 3D pass; disable
	// GL_TEXTURE_2D so the bar's glColor3f calls set flat, untextured color
	// instead of being modulated by whatever texel the untextured quad
	// happens to sample.
	glDisable(GL_TEXTURE_2D);
	Hud::drawBar(30.0f, 6.0f, 40.0f, 4.0f, static_cast<float>(loopRatio), 0.2f, 0.6f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	glDisable(GL_DEPTH_TEST);
	DrawStatsPanel();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1.0f, 1.0f, 1.0f);

	glutSwapBuffers();
}

void Idle() { glutPostRedisplay(); }

void MouseButton(int button, int state, int x, int y) {
	if (button != GLUT_LEFT_BUTTON)
		return;
	if (state == GLUT_DOWN) {
		gDragging = true;
		gLastMouseX = x;
		gLastMouseY = y;
	} else if (state == GLUT_UP) {
		gDragging = false;
	}
}

void MouseMotion(int x, int y) {
	if (!gDragging)
		return;
	int dx = x - gLastMouseX;
	int dy = y - gLastMouseY;
	gLastMouseX = x;
	gLastMouseY = y;

	gYawDeg += static_cast<float>(dx) * DRAG_DEG_PER_PX;
	gPitchDeg += static_cast<float>(dy) * DRAG_DEG_PER_PX; // sign: adjust during
														   // verification if the
														   // up/down feel is
														   // inverted -- not a
														   // hard requirement,
														   // pick whichever reads
														   // as natural by eye
	if (gPitchDeg > MAX_PITCH_DEG)
		gPitchDeg = MAX_PITCH_DEG;
	if (gPitchDeg < -MAX_PITCH_DEG)
		gPitchDeg = -MAX_PITCH_DEG;
}

// Loads the next clip of the model.
void NextClip() {
	// size() <= 1, not empty(): ScanSiblingModels never returns an empty
	// vector (it degrades to {modelPath} on failure/no-match), so a lone-model
	// group (e.g. ankh.md3, sphinx.md3) would otherwise reload the same file on
	// every press -- redundant I/O, a pointless Compile() (leaking a display-list
	// set per press) and a visible progress-bar reset.
	if (gSiblingModelPaths.size() <= 1)
		return;

	std::size_t nextIndex = (gCurrentSiblingIndex + 1) % gSiblingModelPaths.size();
	const std::string& nextPath = gSiblingModelPaths[nextIndex];

	auto next = std::make_unique<LoopedAnimatedModel>();
	if (!next->Load(nextPath.c_str())) {
		LOG_WARNINGF("modelviewer", "Failed to load sibling model: %s", nextPath.c_str());
		return; // keep showing the current model; do not disturb state
	}

	gModel = std::move(next);
	gCurrentSiblingIndex = nextIndex;
	ApplyLoadedModel(nextPath);
}

void NextTexture() {
	if (gTexturePaths.size() <= 1)
		return;
	gTextureIndex = (gTextureIndex + 1) % gTexturePaths.size();
	gModel->BindTexture(LoadCurrentTexture());
}

void KeyPressed(unsigned char key, int /*x*/, int /*y*/) {
	switch (key) {
	case ' ':
		NextClip();
		break;
	case 't':
	case 'T':
		NextTexture();
		break;
	case 'l':
	case 'L':
		SetLightMode(static_cast<LightMode>((static_cast<int>(gLightMode) + 1) % std::size(LIGHT_MODE_NAMES)));
		break;
	case '+':
	case '=': // + without shift
		SetDuration(gDurationSeconds / SPEED_STEP);
		break;
	case '-':
	case '_':
		SetDuration(gDurationSeconds * SPEED_STEP);
		break;
	default:
		break;
	}
}

void Reshape(int width, int height) {
	if (height == 0)
		height = 1;

	gWinWidth = width;
	gWinHeight = height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(width) / static_cast<double>(height), Z_NEAR, Z_FAR);
	glMatrixMode(GL_MODELVIEW);
}

} // namespace

int main(int argc, char* argv[]) {
	Logger::initialize();

	if (argc < 2) {
		std::fprintf(stderr, "Usage: viewer <model-file> [animation-time-seconds] [options], see viewer.cpp\n");
		return 1;
	}

	const std::string modelPath = argv[1];
	double seconds = 5.0;
	int arg = 2;
	if (argc > arg && std::strncmp(argv[arg], "--", 2) != 0) {
		char* end = nullptr;
		seconds = std::strtod(argv[arg], &end);
		if (end == argv[arg] || *end != '\0' || seconds <= 0.0) {
			std::fprintf(stderr, "Invalid animation-time-seconds value: %s (must be a positive number)\n", argv[arg]);
			return 1;
		}
		arg++;
	}
	std::string texture;
	for (; arg < argc; arg++) {
		const std::string option = argv[arg];
		if (arg + 1 >= argc) {
			std::fprintf(stderr, "Missing value for %s\n", option.c_str());
			return 1;
		}
		const char* value = argv[++arg];
		if (option == "--light") {
			auto name = std::find_if(std::begin(LIGHT_MODE_NAMES), std::end(LIGHT_MODE_NAMES),
									 [&](const char* n) { return std::strcmp(n, value) == 0; });
			if (name == std::end(LIGHT_MODE_NAMES)) {
				std::fprintf(stderr, "Unknown light: %s (flat, game or toon)\n", value);
				return 1;
			}
			gLightMode = static_cast<LightMode>(name - std::begin(LIGHT_MODE_NAMES));
		} else if (option == "--yaw") {
			gYawDeg = std::strtof(value, nullptr);
		} else if (option == "--pitch") {
			gPitchDeg = std::clamp(std::strtof(value, nullptr), -MAX_PITCH_DEG, MAX_PITCH_DEG);
		} else if (option == "--texture") {
			texture = value;
		} else if (option == "--shot") {
			gShotDir = value;
		} else if (option == "--frames") {
			if (std::strcmp(value, "all") != 0)
				for (const char* p = value; *p;) {
					char* end = nullptr;
					gShotFrames.push_back(static_cast<int>(std::strtol(p, &end, 10)));
					if (end == p) {
						std::fprintf(stderr, "Invalid --frames: %s\n", value);
						return 1;
					}
					p = *end == ',' ? end + 1 : end;
				}
		} else if (option == "--size") {
			if (std::sscanf(value, "%dx%d", &gWinWidth, &gWinHeight) != 2 || gWinWidth <= 0 || gWinHeight <= 0) {
				std::fprintf(stderr, "Invalid --size: %s (e.g. 800x600)\n", value);
				return 1;
			}
		} else {
			std::fprintf(stderr, "Unknown option: %s\n", option.c_str());
			return 1;
		}
	}
	gDurationSeconds = std::clamp(seconds, MIN_DURATION_SECONDS, MAX_DURATION_SECONDS);

	gModel = std::make_unique<LoopedAnimatedModel>();
	if (!gModel->Load(modelPath.c_str())) {
		std::fprintf(stderr, "Failed to load model: %s\n", modelPath.c_str());
		return 1;
	}

	// glutInit is called only after a successful model load so a bad path
	// never gets as far as opening a window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
	glutInitWindowSize(gWinWidth, gWinHeight);
	glutInitWindowPosition(50, 50);

	std::string title = "Model Viewer :: " + FileStem(modelPath) + ".md3";
	glutCreateWindow(title.c_str());

	InitGL(gWinWidth, gWinHeight);

	gStatsFont.Load("fonts/papyrus.png", 3.6f, 0.1f, true); // the UI screens' body and small fonts
	gHintFont.Load("fonts/papyrus.png", 3.f, 0.08f, true);

	// Sibling-group discovery: scanned exactly once, from the path
	// the user actually typed, and never rescanned. gCurrentSiblingIndex is found by Basename comparison so it
	// doesn't matter whether modelPath and the scanned entries are spelled
	// identically (e.g. "./models/monsters/anubis.md3" vs "models/monsters/anubis.md3").
	gSiblingModelPaths = ScanSiblingModels(modelPath);
	gCurrentSiblingIndex = 0;
	for (std::size_t i = 0; i < gSiblingModelPaths.size(); ++i) {
		if (Basename(gSiblingModelPaths[i]) == Basename(modelPath)) {
			gCurrentSiblingIndex = i;
			break;
		}
	}

	// Call order matches src/entities/monster.cpp, src/entities/trap.cpp and
	// src/state/game_state.cpp -- not src/entities/item.cpp, which swaps
	// Centrify/BindTexture (harmless but not the precedent to follow).
	ApplyLoadedModel(modelPath);
	if (!texture.empty()) {
		auto picked = std::find_if(gTexturePaths.begin(), gTexturePaths.end(),
								   [&](const std::string& path) { return FileStem(path) == texture; });
		if (picked == gTexturePaths.end()) {
			std::fprintf(stderr, "No texture %s for this model\n", texture.c_str());
			return 1;
		}
		gTextureIndex = static_cast<std::size_t>(picked - gTexturePaths.begin());
		gModel->BindTexture(LoadCurrentTexture());
	}
	SetLightMode(gLightMode);

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutReshapeFunc(Reshape);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutKeyboardFunc(KeyPressed);

	glutMainLoop();

	return 0;
}
