// Model viewer: loads a .md3 file with the game's AnimatedModel, plays its animation on a loop that restarts every
// [seconds], with a progress bar of the loop. Space cycles through the model's clips. Runs from the repo root.
//
// Usage: build/model-viewer <model.md3> [seconds]
//   <model.md3>  required, path to a .md3 file (see tools/blender/md3.py)
//   [seconds]    optional, animation loop duration in seconds (default 5.0).

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

#include "../../src/graphics/animated_model.h"
#include "../../src/graphics/textures.h"
#include "hud.h"
#include "../../src/graphics/font.h"
#include "../../src/core/logger.h"
#include "../../src/core/timer.h"

namespace {

// AnimatedModel whose frame follows an external wall-clock ratio instead of Advance().
class LoopedAnimatedModel : public AnimatedModel {
  public:
	// VCount is protected in AnimatedModel. AnimatedModel::Show()/Compile()
	// both draw with glDrawArrays(GL_TRIANGLES, 0, VCount) -- a flat,
	// non-indexed triangle list -- so every 3 vertices are one triangle.
	int TriangleCount() const { return VCount / 3; }

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

float gYawDeg = 20.0f;	// matches the current fixed view exactly, so the
float gPitchDeg = 0.0f; // initial frame on launch is unchanged
bool gDragging = false;
int gLastMouseX = 0;
int gLastMouseY = 0;
const float DRAG_DEG_PER_PX = 0.4f; // empirical; tune by feel
const float MAX_PITCH_DEG = 89.0f;	// avoid flipping past vertical

// Stats-panel state (step 6): loaded/computed once in main() after the model
// finishes loading, since none of these values change after that point.
Font gStatsFont;
std::string gStatModelName;
int gStatFrameCount = 0;
int gStatPlaySpeed = 0;
int gStatPolygonCount = 0;

// Model-state discovery/switching (step 7): the texture object moves to file
// scope so ApplyLoadedModel can reuse it across both the initial load and
// every subsequent [space] switch; the sibling group is scanned once at
// startup and never rescanned (see architecture.md section 1).
Texture gTexture;
std::vector<std::string> gSiblingModelPaths;
std::size_t gCurrentSiblingIndex = 0;
int gStatAnimationStateCount = 0;

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

// Returns the text before the first '_' in a stem, or the whole stem if it
// has none, e.g. ParentStem("anubis_att") -> "anubis", ParentStem("anubis")
// -> "anubis", ParentStem("ankh") -> "ankh".
std::string ParentStem(const std::string& stem) {
	std::size_t underscore = stem.find('_');
	return (underscore == std::string::npos) ? stem : stem.substr(0, underscore);
}

// Every *.md3 in modelPath's directory with the same parent stem (its clips), sorted by filename.
// Always includes modelPath itself.
std::vector<std::string> ScanSiblingModels(const std::string& modelPath) {
	std::filesystem::path path(modelPath);
	std::filesystem::path dir = path.parent_path();
	if (dir.empty())
		dir = ".";
	const std::string parentStem = ParentStem(FileStem(modelPath));
	const std::string loadedBasename = path.filename().string();
	std::vector<std::string> result;
	bool foundLoadedFile = false;

	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		if (!entry.is_regular_file())
			continue;
		const bool isLoadedFile = entry.path().filename().string() == loadedBasename;
		if (!isLoadedFile &&
			(entry.path().extension() != ".md3" || ParentStem(entry.path().stem().string()) != parentStem))
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

// Best-effort texture fallback chain:
//   1. textures/<category>/<texture-stem>.png, where <category> is the model's
//      sub-directory under models/ (e.g. models/monsters/anubis.md3 ->
//      textures/monsters/anubis.png)
//   2. textures/null.png
//   3. untextured (id 0)
// Every failure is logged as a warning, never fatal -- a missing/wrong
// texture must never prevent seeing the animation. textureStem is already
// resolved by the caller (ParentStem(FileStem(path))): a variant's
// texture always comes from its parent's stem, not its own.
int LoadTextureForModel(const std::string& modelPath, const std::string& textureStem, Texture& tex) {
	std::string category = std::filesystem::path(modelPath).parent_path().filename().string();
	std::string guess = "textures/" + category + "/" + textureStem + ".png";
	if (tex.LoadPNG(guess.c_str())) {
		return tex.ID();
	}

	LOG_WARNINGF("modelviewer", "No texture found at %s, falling back to textures/null.png", guess.c_str());
	if (tex.LoadPNG("textures/null.png")) {
		return tex.ID();
	}

	LOG_WARNINGF("modelviewer", "%s", "textures/null.png fallback also failed, continuing untextured");
	return 0;
}

// gModel must already have a successful Load() by the time this runs --
// called once from main() for the initial model, and again from
// KeyPressed() on every [space] cycle. Resets the loop timer and stats;
// deliberately leaves gDurationSeconds/gYawDeg/gPitchDeg untouched.
void ApplyLoadedModel(const std::string& path) {
	std::string textureStem = ParentStem(FileStem(path));
	int texId = LoadTextureForModel(path, textureStem, gTexture);

	gModel->BindTexture(texId);
	gModel->Centrify();
	gModel->Compile();

	gStartTicks = GameClock::now();

	gStatModelName = Basename(path);
	gStatFrameCount = gModel->FrameCount();
	gStatPolygonCount = gModel->TriangleCount();
	gStatAnimationStateCount = gSiblingModelPaths.empty() ? 0 : static_cast<int>(gSiblingModelPaths.size()) - 1;
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
	gluPerspective(45.0, static_cast<double>(width) / static_cast<double>(height), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Display() {
	double elapsedSeconds = (GameClock::now() - gStartTicks) / 1000.0;
	double loopRatio = std::fmod(elapsedSeconds, gDurationSeconds) / gDurationSeconds;
	if (gModel)
		gModel->SetProgress(static_cast<float>(loopRatio));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Re-issue the 3D perspective projection every frame (mirrors
	// src/graphics/draw.cpp's Draw(), which also sets up gluPerspective from
	// scratch each call) since the 2D overlay pass below switches the
	// projection matrix to an orthographic one for the HUD bar.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(gWinWidth) / static_cast<double>(gWinHeight), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Centrify() normalizes the model to roughly unit size with its base at
	// y=0 and centered on x/z, so a fixed camera a couple of units back and
	// half a unit down (to vertically center the ~1-unit-tall model) frames
	// it reasonably for any model. A static yaw gives a 3/4 view instead of
	// a flat front-on silhouette; nothing here animates the model itself.
	glTranslatef(-0.35f, -0.5f, -2.2f);
	glRotatef(gYawDeg, 0.0f, 1.0f, 0.0f);
	glRotatef(gPitchDeg, 1.0f, 0.0f, 0.0f);

	if (gModel)
		gModel->Show();

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

	// Stats panel (step 6). Font::print draws textured glyph quads, so this
	// must come after the bar's glEnable(GL_TEXTURE_2D) above, not inside the
	// disabled block. glColor3f(1,1,1) undoes Hud::drawBar's last fill color
	// (blue) so the text isn't tinted; the blend func/enable is required
	// because fonts/papyrus_i.png has no alpha channel, so without blending
	// each glyph quad would draw as a solid-colored box instead of legible
	// text. glDisable(GL_BLEND) must run before this function returns so it
	// doesn't leak into the next frame's opaque 3D model draw.
	glColor3f(1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	glEnable(GL_BLEND);
	gStatsFont.print(56, 92, "Name: %s", gStatModelName.c_str());
	gStatsFont.print(56, 83, "Frames: %d", gStatFrameCount);
	gStatsFont.print(56, 74, "Play Speed: %d s", gStatPlaySpeed);
	gStatsFont.print(56, 65, "Polygon count: %d", gStatPolygonCount);
	gStatsFont.print(56, 56, "Animation states: %d", gStatAnimationStateCount);
	glDisable(GL_BLEND);

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

void KeyPressed(unsigned char key, int /*x*/, int /*y*/) {
	// size() <= 1, not empty(): ScanSiblingModels never returns an empty
	// vector (it degrades to {modelPath} on failure/no-match), so a bare
	// empty() check would never actually fire, and a lone-model group
	// (e.g. ankh.md3, sphinx.md3) would fall through to reloading the
	// exact same file on every press -- redundant I/O, a pointless
	// Compile()/BindTexture() (leaking one GL texture + display-list set
	// per press), and a visible progress-
	// bar reset the "Animation states: 0" line explicitly promises won't
	// happen. size() <= 1 is what actually makes a lone-model group a
	// true no-op.
	if (key != ' ' || gSiblingModelPaths.size() <= 1)
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

void Reshape(int width, int height) {
	if (height == 0)
		height = 1;

	gWinWidth = width;
	gWinHeight = height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(width) / static_cast<double>(height), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
}

} // namespace

int main(int argc, char* argv[]) {
	Logger::initialize();

	if (argc < 2) {
		std::fprintf(stderr, "Usage: viewer <model-file> [animation-time-seconds]\n");
		return 1;
	}

	const std::string modelPath = argv[1];
	double seconds = 5.0;
	if (argc >= 3) {
		char* end = nullptr;
		seconds = std::strtod(argv[2], &end);
		if (end == argv[2] || *end != '\0' || seconds <= 0.0) {
			std::fprintf(stderr, "Invalid animation-time-seconds value: %s (must be a positive number)\n", argv[2]);
			return 1;
		}
	}
	gDurationSeconds = seconds;

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

	gStatsFont.Load("fonts/papyrus_i.png", 5, -0.6); // matches src/ui/stats.cpp's
													 // Impact-font convention
	gStatPlaySpeed = static_cast<int>(gDurationSeconds);

	// Sibling-group discovery (step 7): scanned exactly once, from the path
	// the user actually typed, and never rescanned -- see architecture.md
	// section 1. gCurrentSiblingIndex is found by Basename comparison so it
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

	glutDisplayFunc(Display);
	glutIdleFunc(Idle);
	glutReshapeFunc(Reshape);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutKeyboardFunc(KeyPressed);

	glutMainLoop();

	return 0;
}
