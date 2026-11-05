// Disarm trap's red contour (plan heka-surplus, the rules in dungeon_seshat.cpp) round a trapped container or a
// disguised mimic near the player: the model drawn again, only the edges of its back faces, as thick lines in plain
// red. Where the front faces cover them only the silhouette's rim shows. Unlit: it shows in the dark.
#include "dungeon.h"
#include "../state/assets.h"
#include "../graphics/lighting.h"
#include <GL/gl.h>

namespace {
constexpr float CONTOUR_ALPHA = 1.f;
constexpr float CONTOUR_COLOUR[3] = {1.f, 0.15f, 0.08f};
constexpr float CONTOUR_PX = 7.f; // line width at 720 lines of screen, as wide a share at any size
constexpr float REFERENCE_HEIGHT = 720.f;
constexpr float CONTOUR_OFFSET = 2.f;
} // namespace

void Dungeon::drawContour(float strength, const std::function<void(int texture)>& draw) const {
	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	glPushAttrib(
		GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT
	);
	Lighting::setEmissive(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glPolygonMode(GL_BACK, GL_LINE);
	glEnable(GL_LINE_SMOOTH);
	// Pushed back a little: the box edges where a back face meets a front one hide, the rim stays.
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(CONTOUR_OFFSET, CONTOUR_OFFSET);
	glLineWidth(CONTOUR_PX * static_cast<float>(viewport[3]) / REFERENCE_HEIGHT);
	glColor4f(CONTOUR_COLOUR[0], CONTOUR_COLOUR[1], CONTOUR_COLOUR[2], CONTOUR_ALPHA * strength);
	draw(sim.assets->textures.nullTex.ID());
	Lighting::setEmissive(false);
	glPopAttrib();
	glColor3f(1, 1, 1);
}
