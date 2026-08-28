// ModelViewer :: viewer.cpp
//
// Steps 1-3 of the model-viewer-executable plan: open a GLUT window, load a
// single .mdl file named on the command line via the existing AnimatedModel
// class, play its animation on a timed loop that restarts every [seconds],
// and overlay a 2D progress bar (via the shared Hud::drawBar) showing how
// far through the current loop the animation is.
//
// Usage: viewer <model.mdl> [seconds]
//   <model.mdl>  required, path to a .mdl file (see src/graphics/ani.cpp)
//   [seconds]    optional, animation loop duration in seconds (default 5.0).

#include <SDL/SDL.h>
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

#include "../src/graphics/ani.h"
#include "../src/graphics/textures.h"
#include "../src/graphics/hud.h"
#include "../src/graphics/font.h"
#include "../src/core/logger.h"

namespace {

// Subclass of AnimatedModel that maps an external wall-clock ratio directly
// onto the model's (protected) frame index, bypassing Advance_Animation()/
// the internal frameChange timer entirely. See
// plan/model-viewer-executable/step-02-timed-loop-animation-playback/architecture.md
// for the full rationale.
class LoopedAnimatedModel : public AnimatedModel {
  public:
	// frameC is protected in AnimatedModel; this is read-only access to it.
	int FrameCount() const { return frameC; }

	// VCount is protected in AnimatedModel. AnimatedModel::Show()/Compile()
	// both draw with glDrawArrays(GL_TRIANGLES, 0, VCount) -- a flat,
	// non-indexed triangle list -- so every 3 vertices are one triangle.
	int TriangleCount() const { return VCount / 3; }

	// Maps ratio in [0, 1) onto frame in [0, frameC), bypassing
	// Advance_Animation()/frameChange entirely.
	void SetProgress(float ratio) {
		if (frameC <= 1) {
			frame = 0.0f;
			return;
		}
		if (ratio < 0.0f)
			ratio = 0.0f;
		else if (ratio > 1.0f)
			ratio = 1.0f;

		frame = ratio * static_cast<float>(frameC);
		if (frame >= static_cast<float>(frameC))
			frame = static_cast<float>(frameC) - 1.0f;
	}
};

std::unique_ptr<LoopedAnimatedModel> g_model;
int g_winWidth = 800;
int g_winHeight = 600;
Uint32 g_startTicks = 0;
double g_durationSeconds = 5.0;

float g_yawDeg = 20.0f;   // matches the current fixed view exactly, so the
float g_pitchDeg = 0.0f;  // initial frame on launch is unchanged
bool g_dragging = false;
int g_lastMouseX = 0;
int g_lastMouseY = 0;
const float kDragSensitivityDegPerPx = 0.4f; // empirical; tune by feel
const float kMaxPitchDeg = 89.0f;             // avoid flipping past vertical

// Stats-panel state (step 6): loaded/computed once in main() after the model
// finishes loading, since none of these values change after that point.
Font g_statsFont;
std::string g_statModelName;
int g_statFrameCount = 0;
int g_statPlaySpeed = 0;
int g_statPolygonCount = 0;

// Model-state discovery/switching (step 7): the texture object moves to file
// scope so ApplyLoadedModel can reuse it across both the initial load and
// every subsequent [space] switch; the sibling group is scanned once at
// startup and never rescanned (see architecture.md section 1).
Textura g_texture;
std::vector<std::string> g_siblingModelPaths;
std::size_t g_currentSiblingIndex = 0;
int g_statAnimationStateCount = 0;

// Returns the filename with its directory stripped but extension kept,
// e.g. "Models/anubis.mdl" -> "anubis.mdl". Distinct from FileStem()
// (which also strips the extension, for the window title's own use).
std::string Basename(const std::string& path) {
	std::size_t slash = path.find_last_of("/\\");
	return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

// Returns the filename stem (no directory, no extension) of a path, e.g.
// "Models/anubis.mdl" -> "anubis". Used only for the best-effort
// model-stem -> texture-stem convention described in step.md; this repo has
// no general model->texture mapping to reuse.
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
// -> "anubis", ParentStem("ankh") -> "ankh". See step.md's "Parent/variant
// detection rule".
std::string ParentStem(const std::string& stem) {
	std::size_t underscore = stem.find('_');
	return (underscore == std::string::npos) ? stem : stem.substr(0, underscore);
}

// Finds every *.mdl file in modelPath's own directory that shares its parent
// stem (see step.md's "Parent/variant detection rule"), sorted by filename.
// Always includes modelPath itself. Excludes any candidate whose suffix
// (text after its first '_') is the literal token "old" (case-insensitive)
// -- a reserved marker for stray backup files, see step.md's sphinx_old.mdl
// research. Directory listing failures (should not happen for a path that
// already Load()-ed successfully) degrade to a group of one (just
// modelPath) rather than throwing.
std::vector<std::string> ScanSiblingModels(const std::string& modelPath) {
	std::filesystem::path path(modelPath);
	std::filesystem::path dir = path.parent_path();
	if (dir.empty())
		dir = ".";

	// If the loaded file itself would fail the extension filter (rule 1) or
	// the old-suffix filter (rule 3) were it being evaluated as a plain
	// candidate, it is a reserved/stray file by those same rules and must
	// not derive a shared parent stem that could pull in unrelated real
	// family members -- e.g. sphinx_old.mdl's own stem reduces to parent
	// stem "sphinx" via ParentStem(), which would otherwise match the
	// genuine sphinx.mdl and incorrectly merge them into one group; the
	// existing rule-3 exclusion only ever protects a *candidate* from being
	// pulled into someone else's group, it never revisits the loaded file's
	// own derived parent stem. Isolating here keeps the group symmetric:
	// a reserved/stray file's group is always exactly {itself}.
	std::string ownStem = FileStem(modelPath);
	bool ownExtensionFailsRule1 = (path.extension() != ".mdl");
	bool ownStemFailsRule3 = false;
	{
		std::size_t ownUnderscore = ownStem.find('_');
		if (ownUnderscore != std::string::npos) {
			std::string ownSuffix = ownStem.substr(ownUnderscore + 1);
			for (char& c : ownSuffix) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			if (ownSuffix == "old")
				ownStemFailsRule3 = true;
		}
	}
	if (ownExtensionFailsRule1 || ownStemFailsRule3)
		return {modelPath};

	std::string parentStem = ParentStem(FileStem(modelPath));
	std::string loadedBasename = path.filename().string();
	std::vector<std::string> result;
	bool foundLoadedFile = false;

	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		if (ec)
			break;
		if (!entry.is_regular_file())
			continue;

		// The loaded file is always a member of its own group, even if it
		// would otherwise fail the extension or old-suffix filters below
		// (e.g. the user explicitly names Models/columns.mdl_old or
		// Models/sphinx_old.mdl on the command line -- both load fine via
		// AnimatedModel::Load, which does not check extension). Those
		// filters exist to keep OTHER candidates out of a group the user
		// didn't ask to see; they must never evict the file the user
		// actually loaded.
		bool isLoadedFile = (entry.path().filename().string() == loadedBasename);

		if (!isLoadedFile) {
			if (entry.path().extension() != ".mdl")
				continue;

			std::string candidateStem = entry.path().stem().string();
			if (ParentStem(candidateStem) != parentStem)
				continue;

			std::size_t underscore = candidateStem.find('_');
			if (underscore != std::string::npos) {
				std::string suffix = candidateStem.substr(underscore + 1);
				for (char& c : suffix) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				if (suffix == "old")
					continue; // reserved backup-file marker, see step.md
			}
		}

		if (isLoadedFile)
			foundLoadedFile = true;
		result.push_back((dir / entry.path().filename()).string());
	}

	if (!foundLoadedFile)
		result.push_back(modelPath); // directory iteration somehow missed
		                               // the loaded file (or excluded it) --
		                               // add it back rather than silently
		                               // dropping the model the user loaded

	if (ec || result.empty())
		return {modelPath}; // directory read failed or found nothing --
		                     // degrade to a group of one rather than
		                     // losing the model the user actually loaded
	std::sort(result.begin(), result.end());
	return result;
}

// Best-effort texture fallback chain (step.md):
//   1. Textures/<texture-stem>.bmp
//   2. Textures/null.bmp
//   3. untextured (id 0)
// Every failure is logged as a warning, never fatal -- a missing/wrong
// texture must never prevent seeing the animation. textureStem is already
// resolved by the caller (ParentStem(FileStem(path))) -- see step.md's
// "Existing texture-fallback chain to extend, not duplicate": a variant's
// texture always comes from its parent's stem, not its own.
int LoadTextureForModel(const std::string& textureStem, Textura& tex) {
	std::string guess = "Textures/" + textureStem + ".bmp";
	if (tex.LoadBMP(guess.c_str())) {
		return tex.ID();
	}

	LOG_WARNINGF("modelviewer", "No texture found at %s, falling back to Textures/null.bmp", guess.c_str());
	if (tex.LoadBMP("Textures/null.bmp")) {
		return tex.ID();
	}

	LOG_WARNINGF("modelviewer", "%s", "Textures/null.bmp fallback also failed, continuing untextured");
	return 0;
}

// g_model must already have a successful Load() by the time this runs --
// called once from main() for the initial model, and again from
// KeyPressed() on every [space] cycle. Resets the loop timer and stats;
// deliberately leaves g_durationSeconds/g_yawDeg/g_pitchDeg untouched (see
// step.md's "what carries over" section).
void ApplyLoadedModel(const std::string& path) {
	std::string textureStem = ParentStem(FileStem(path));
	int texId = LoadTextureForModel(textureStem, g_texture);

	g_model->BindTexture(texId);
	g_model->Centrify();
	g_model->Compile();

	g_startTicks = SDL_GetTicks();

	g_statModelName = Basename(path);
	g_statFrameCount = g_model->FrameCount();
	g_statPolygonCount = g_model->TriangleCount();
	g_statAnimationStateCount = g_siblingModelPaths.empty()
	    ? 0
	    : static_cast<int>(g_siblingModelPaths.size()) - 1;
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
	double elapsedSeconds = (SDL_GetTicks() - g_startTicks) / 1000.0;
	double loopRatio = std::fmod(elapsedSeconds, g_durationSeconds) / g_durationSeconds;
	if (g_model)
		g_model->SetProgress(static_cast<float>(loopRatio));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Re-issue the 3D perspective projection every frame (mirrors
	// src/graphics/draw.cpp's Draw(), which also sets up gluPerspective from
	// scratch each call) since the 2D overlay pass below switches the
	// projection matrix to an orthographic one for the HUD bar.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<double>(g_winWidth) / static_cast<double>(g_winHeight), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Centrify() normalizes the model to roughly unit size with its base at
	// y=0 and centered on x/z, so a fixed camera a couple of units back and
	// half a unit down (to vertically center the ~1-unit-tall model) frames
	// it reasonably for any model. A static yaw gives a 3/4 view instead of
	// a flat front-on silhouette; nothing here animates the model itself.
	glTranslatef(-0.35f, -0.5f, -2.2f);
	glRotatef(g_yawDeg, 0.0f, 1.0f, 0.0f);
	glRotatef(g_pitchDeg, 1.0f, 0.0f, 0.0f);

	if (g_model)
		g_model->Show();

	// 2D loop-progress bar overlay, using the same
	// glOrtho(0, 100, 0, 100, -21, 21) 2D-overlay convention
	// src/graphics/draw.cpp's Draw() uses for the in-game HUD, drawn via the
	// same Hud::drawBar the shipped game's health/stamina bars use.
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
	// because Fonts/papyrus_i.bmp has no alpha channel, so without blending
	// each glyph quad would draw as a solid-colored box instead of legible
	// text. glDisable(GL_BLEND) must run before this function returns so it
	// doesn't leak into the next frame's opaque 3D model draw.
	glColor3f(1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	glEnable(GL_BLEND);
	g_statsFont.print(56, 92, "Name: %s", g_statModelName.c_str());
	g_statsFont.print(56, 83, "Frames: %d", g_statFrameCount);
	g_statsFont.print(56, 74, "Play Speed: %d s", g_statPlaySpeed);
	g_statsFont.print(56, 65, "Polygon count: %d", g_statPolygonCount);
	g_statsFont.print(56, 56, "Animation states: %d", g_statAnimationStateCount);
	glDisable(GL_BLEND);

	glutSwapBuffers();
}

void Idle() { glutPostRedisplay(); }

void MouseButton(int button, int state, int x, int y) {
	if (button != GLUT_LEFT_BUTTON)
		return;
	if (state == GLUT_DOWN) {
		g_dragging = true;
		g_lastMouseX = x;
		g_lastMouseY = y;
	} else if (state == GLUT_UP) {
		g_dragging = false;
	}
}

void MouseMotion(int x, int y) {
	if (!g_dragging)
		return;
	int dx = x - g_lastMouseX;
	int dy = y - g_lastMouseY;
	g_lastMouseX = x;
	g_lastMouseY = y;

	g_yawDeg += dx * kDragSensitivityDegPerPx;
	g_pitchDeg += dy * kDragSensitivityDegPerPx; // sign: adjust during
	                                              // verification if the
	                                              // up/down feel is
	                                              // inverted -- not a
	                                              // hard requirement,
	                                              // pick whichever reads
	                                              // as natural by eye
	if (g_pitchDeg > kMaxPitchDeg)
		g_pitchDeg = kMaxPitchDeg;
	if (g_pitchDeg < -kMaxPitchDeg)
		g_pitchDeg = -kMaxPitchDeg;
}

void KeyPressed(unsigned char key, int /*x*/, int /*y*/) {
	// size() <= 1, not empty(): ScanSiblingModels never returns an empty
	// vector (it degrades to {modelPath} on failure/no-match), so a bare
	// empty() check would never actually fire, and a lone-model group
	// (e.g. ankh.mdl, sphinx.mdl) would fall through to reloading the
	// exact same file on every press -- redundant I/O, a pointless
	// Compile()/BindTexture() (leaking one GL texture + display-list set
	// per press, see step.md's known-leak note), and a visible progress-
	// bar reset the "Animation states: 0" line explicitly promises won't
	// happen. size() <= 1 is what actually makes a lone-model group a
	// true no-op.
	if (key != ' ' || g_siblingModelPaths.size() <= 1)
		return;

	std::size_t nextIndex = (g_currentSiblingIndex + 1) % g_siblingModelPaths.size();
	const std::string& nextPath = g_siblingModelPaths[nextIndex];

	auto next = std::make_unique<LoopedAnimatedModel>();
	if (!next->Load(nextPath.c_str())) {
		LOG_WARNINGF("modelviewer", "Failed to load sibling model: %s", nextPath.c_str());
		return; // keep showing the current model; do not disturb state
	}

	g_model = std::move(next);
	g_currentSiblingIndex = nextIndex;
	ApplyLoadedModel(nextPath);
}

void Reshape(int width, int height) {
	if (height == 0)
		height = 1;

	g_winWidth = width;
	g_winHeight = height;

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
	g_durationSeconds = seconds;

	if (SDL_Init(SDL_INIT_TIMER) < 0) {
		std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	// AnimatedModel's constructor creates a timer internally, which calls
	// SDL_GetTicks() -- SDL_Init must have already run before this point.
	g_model = std::make_unique<LoopedAnimatedModel>();
	if (!g_model->Load(modelPath.c_str())) {
		std::fprintf(stderr, "Failed to load model: %s\n", modelPath.c_str());
		return 1;
	}

	// glutInit is called only after a successful model load so a bad path
	// never gets as far as opening a window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
	glutInitWindowSize(g_winWidth, g_winHeight);
	glutInitWindowPosition(50, 50);

	std::string title = "Model Viewer :: " + FileStem(modelPath) + ".mdl";
	glutCreateWindow(title.c_str());

	InitGL(g_winWidth, g_winHeight);

	g_statsFont.Load("Fonts/papyrus_i.bmp", 5, -0.6); // matches src/ui/stats.cpp's
	                                                    // Impact-font convention
	g_statPlaySpeed = static_cast<int>(g_durationSeconds);

	// Sibling-group discovery (step 7): scanned exactly once, from the path
	// the user actually typed, and never rescanned -- see architecture.md
	// section 1. g_currentSiblingIndex is found by Basename comparison so it
	// doesn't matter whether modelPath and the scanned entries are spelled
	// identically (e.g. "./Models/anubis.mdl" vs "Models/anubis.mdl").
	g_siblingModelPaths = ScanSiblingModels(modelPath);
	g_currentSiblingIndex = 0;
	for (std::size_t i = 0; i < g_siblingModelPaths.size(); ++i) {
		if (Basename(g_siblingModelPaths[i]) == Basename(modelPath)) {
			g_currentSiblingIndex = i;
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
