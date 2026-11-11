#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/core/worker_pool.h"
#include "../../src/sim/md3_mesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <vector>

// Plan faster-loading: the mesh keeps the MD3 indexing. The numbers are those of the corner-per-triangle mesh before.
TEST_CASE("an indexed MD3 mesh measures as the expanded one did") {
	Md3Mesh mesh;
	REQUIRE(mesh.Load(MEDIA_DIR "models/monsters/ushabti.md3"));
	CHECK(mesh.frameCount == 20);
	CHECK(mesh.TriangleCount() * 3 == 3720);
	CHECK(mesh.vertexCount > 0);
	CHECK(mesh.vertexCount <= 3720);
	CHECK(mesh.texCoords.size() == 2 * static_cast<size_t>(mesh.vertexCount));
	for (const VF& frame : mesh.frames) {
		CHECK(frame.v.size() == 3 * static_cast<size_t>(mesh.vertexCount));
		CHECK(frame.n.size() == 3 * static_cast<size_t>(mesh.vertexCount));
	}
	for (uint32_t i : mesh.indices)
		CHECK(i < static_cast<uint32_t>(mesh.vertexCount));
	CHECK(mesh.YRange(0).first == 6.33368909e-05f);
	CHECK(mesh.YRange(0).second == 0.517589092f);
	CHECK(mesh.HalfXZ(0).first == 0.118994184f);
	CHECK(mesh.HalfXZ(0).second == 0.186242133f);
	CHECK(mesh.YRange(19).first == 0.f);
	CHECK(mesh.YRange(19).second == 0.517842412f);
}

TEST_CASE("the MD3 normal table holds the lat/long formula of every code") {
	const float step = 2.0f * static_cast<float>(M_PI) / 255.0f;
	for (int code = 0; code < 65536; code++) {
		const float azimuth = static_cast<float>(code >> 8) * step;
		const float polar = static_cast<float>(code & 0xFF) * step;
		const float* n = Md3DecodeNormal(static_cast<int16_t>(static_cast<uint16_t>(code)));
		REQUIRE(n[0] == cosf(azimuth) * sinf(polar));
		REQUIRE(n[1] == sinf(azimuth) * sinf(polar));
		REQUIRE(n[2] == cosf(polar));
	}
}

TEST_CASE("the worker pool runs every job and hands back its result") {
	WorkerPool pool(3);
	std::vector<std::future<int>> results;
	for (int i = 0; i < 50; i++)
		results.push_back(pool.Run([i] { return i * i; }));
	for (int i = 0; i < 50; i++)
		CHECK(results[static_cast<size_t>(i)].get() == i * i);
}

TEST_CASE("the player's idle keeps both feet planted (plan player-idle-stance)") {
	Md3Mesh mesh;
	REQUIRE(mesh.Load(MEDIA_DIR "models/characters/archeologist.md3"));
	mesh.Centrify(); // frame 0 height 1 (about 1.9 m)
	REQUIRE(mesh.frameCount > 1);
	int feet = 0;
	float drift = 0.f; // the farthest a boot vertex moves from its frame 0 place
	for (int i = 0; i < mesh.vertexCount; i++) {
		const std::array<float, 3> rest = mesh.Vertex(0, i);
		if (rest[1] > 0.04f) // the boots below the ankles
			continue;
		feet++;
		for (int f = 1; f < mesh.frameCount; f++) {
			const std::array<float, 3> v = mesh.Vertex(f, i);
			drift = std::max(drift, std::hypot(v[0] - rest[0], v[1] - rest[1], v[2] - rest[2]));
		}
	}
	CHECK(feet > 0);
	CHECK(drift < 0.005f); // under 1 cm
}
