// levelconvert: rewrites level files in the current format (LEVEL_VERSION, docs/levels.md), in place.
//   levelconvert FILE...
// A file already in the current format is left as it is. Exit code: 0 done, 2 usage / unreadable or unwritable file.

#include "../../src/level/level.h"
#include <cstdio>
#include <print>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::println(stderr, "usage: levelconvert FILE...");
		return 2;
	}
	int code = 0;
	for (int i = 1; i < argc; i++) {
		LevelGrid grid;
		int version = 0;
		std::string error = loadLevelFile(argv[i], grid, &version);
		if (!error.empty()) {
			std::println(stderr, "{}: {}", argv[i], error);
			code = 2;
			continue;
		}
		if (version == LEVEL_VERSION) {
			std::println("{}: already v{}", argv[i], LEVEL_VERSION);
			continue;
		}
		if (!saveLevelFile(argv[i], grid)) {
			std::println(stderr, "{}: cannot write", argv[i]);
			code = 2;
			continue;
		}
		std::println("{}: v{} -> v{}", argv[i], version, LEVEL_VERSION);
	}
	return code;
}
