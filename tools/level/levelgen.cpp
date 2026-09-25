// levelgen: writes random levels. See docs/levels.md.
//   levelgen [--seed N] [--difficulty D] [--count K] [--map] OUT
// With --count K > 1, OUT is a prefix: OUT1 .. OUTK, seeds N .. N+K-1.

#include "../../src/world/level.h"
#include "../../src/world/level_check.h"
#include "../../src/world/level_gen.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
int usage() {
	fprintf(stderr, "usage: levelgen [--seed N] [--difficulty 1-10] [--count K] [--map] OUT\n");
	return 2;
}
} // namespace

int main(int argc, char** argv) {
	GenOptions options;
	int count = 1;
	bool map = false;
	const char* out = nullptr;
	for (int i = 1; i < argc; i++) {
		bool hasValue = i + 1 < argc;
		if (strcmp(argv[i], "--seed") == 0 && hasValue)
			options.seed = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 10));
		else if (strcmp(argv[i], "--difficulty") == 0 && hasValue)
			options.difficulty = atoi(argv[++i]);
		else if (strcmp(argv[i], "--count") == 0 && hasValue)
			count = atoi(argv[++i]);
		else if (strcmp(argv[i], "--map") == 0)
			map = true;
		else if (argv[i][0] != '-' && out == nullptr)
			out = argv[i];
		else
			return usage();
	}
	if (out == nullptr || count < 1 || options.difficulty < GEN_MIN_DIFFICULTY ||
		options.difficulty > GEN_MAX_DIFFICULTY)
		return usage();

	int code = 0;
	for (int k = 0; k < count; k++) {
		GenOptions o = options;
		o.seed = options.seed + static_cast<uint32_t>(k);
		GenResult result = generateLevel(o);
		std::string path = count > 1 ? std::string(out) + std::to_string(k + 1) : std::string(out);
		if (!result.ok) {
			fprintf(stderr, "%s: no valid level for seed %u after %d attempts\n", path.c_str(), o.seed,
					result.attempts);
			code = 1;
			continue;
		}
		if (!saveLevelFile(path.c_str(), result.grid)) {
			fprintf(stderr, "%s: cannot write\n", path.c_str());
			return 2;
		}
		const LevelReport& r = result.report;
		printf("%s: seed %u, difficulty %d -> score %.1f (target %.1f, %d attempts), path %d, %d monsters, "
			   "%d gates\n",
			   path.c_str(), o.seed, o.difficulty, r.difficulty, result.target, result.attempts, r.pathLength,
			   r.monsterCount, r.gates);
		if (map)
			printf("%s", renderLevel(result.grid, &r).c_str());
	}
	return code;
}
