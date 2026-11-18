// Disarm trap's red contour (plan heka-surplus, the rules in dungeon_seshat.cpp) round a trapped container or a
// disguised mimic near the player: the model drawn again, only the edges of its back faces, as thick lines in plain
// red. Where the front faces cover them only the silhouette's rim shows. Soft: CONTOUR_WIDTHS passes from wide to
// narrow, each faint, so the rim is about CONTOUR_CORE opaque at its line and fades outward. Unlit: shows in the dark.
// The many edges along a rim would pile their alpha up: each pass writes the depth and tests GL_LESS, so a pixel takes
// one line of a pass, the next pass a little nearer (its polygon offset) to come over it. That depth would hide what
// is drawn behind later, so the contours wait for the end of the opaque scene (queueContour, drawContours).
#include "dungeon_view.h"
#include "world_draw.h"
#include "assets.h"
#include "../render/lighting.h"
#include "../render/gl_includes.h"
#include <cmath>
#include <iterator>
#include <utility>

namespace {
constexpr float CONTOUR_COLOUR[3] = {1.f, 0.15f, 0.08f};
// Line widths at 720 lines of screen (as wide a share at any size), the widest first.
constexpr float CONTOUR_WIDTHS[] = {14.f, 9.f, 5.f};
constexpr int CONTOUR_PASSES = static_cast<int>(std::size(CONTOUR_WIDTHS));
constexpr float CONTOUR_CORE = 0.2f; // the opacity where every pass overlaps
constexpr float REFERENCE_HEIGHT = 720.f;
// Polygon offset of the widest pass, one step nearer each pass after it: behind the front faces, so the box edges
// where a back face meets a front one hide and the rim stays.
constexpr float CONTOUR_OFFSET = 2.f * CONTOUR_PASSES;
constexpr float CONTOUR_OFFSET_STEP = 2.f;
} // namespace

void DungeonView::queueContour(float strength, std::function<void(int texture)> draw) {
	Contour c{{}, strength, std::move(draw)};
	c.modelview = gMv.top().m;
	contours.push_back(std::move(c));
}
void DungeonView::drawContours() {
	if (contours.empty())
		return;
	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	// What the passes change, put back after them (was glPushAttrib).
	const GlState saved = GlState::read();
	const BatchVertex current = world().currentValues();
	GLint depthFunc = GL_LESS, cullFace = GL_BACK;
	GLint polygonMode[2] = {GL_FILL, GL_FILL};
	GLfloat lineWidth = 1.f, offsetFactor = 0.f, offsetUnits = 0.f;
	glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
	glGetIntegerv(GL_CULL_FACE_MODE, &cullFace);
	glGetIntegerv(GL_POLYGON_MODE, polygonMode);
	glGetFloatv(GL_LINE_WIDTH, &lineWidth);
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &offsetFactor);
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &offsetUnits);
	const bool lineSmooth = glIsEnabled(GL_LINE_SMOOTH) == GL_TRUE;
	const bool offsetLine = glIsEnabled(GL_POLYGON_OFFSET_LINE) == GL_TRUE;
	gMv.push();
	Lighting::setEmissive(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glPolygonMode(GL_BACK, GL_LINE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_OFFSET_LINE);
	// Each pass's alpha a, so that the passes over each other make 1 - (1 - a)^n = CONTOUR_CORE.
	const float alpha = 1.f - std::pow(1.f - CONTOUR_CORE, 1.f / static_cast<float>(CONTOUR_PASSES));
	const int white = assets.textures.nullTex.ID();
	for (const Contour& c : contours) {
		gMv.load(Mat4::fromArray(c.modelview.data()));
		world().colour(CONTOUR_COLOUR[0], CONTOUR_COLOUR[1], CONTOUR_COLOUR[2], alpha * c.strength);
		float offset = CONTOUR_OFFSET;
		for (const float width : CONTOUR_WIDTHS) {
			glPolygonOffset(offset, offset);
			glLineWidth(width * static_cast<float>(viewport[3]) / REFERENCE_HEIGHT);
			c.draw(white);
			offset -= CONTOUR_OFFSET_STEP;
		}
	}
	contours.clear();
	Lighting::setEmissive(false);
	gMv.pop();
	saved.apply();
	glDepthFunc(static_cast<GLenum>(depthFunc));
	glCullFace(static_cast<GLenum>(cullFace));
	glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode[0]));
	glLineWidth(lineWidth);
	glPolygonOffset(offsetFactor, offsetUnits);
	if (!lineSmooth)
		glDisable(GL_LINE_SMOOTH);
	if (!offsetLine)
		glDisable(GL_POLYGON_OFFSET_LINE);
	world().colour(current.r, current.g, current.b, current.a);
	world().normal(current.nx, current.ny, current.nz);
	world().texCoord(current.u, current.v);
	world().colour(1, 1, 1);
}
