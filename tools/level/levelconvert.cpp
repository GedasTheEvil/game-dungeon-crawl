// levelconvert: rewrites level files in the current format (level format v2, docs/levels.md), in place.
//   levelconvert FILE...
// A file already in the current format is left as it is. Exit code: 0 done, 2 usage / unreadable or unwritable file.

#include "../../src/world/level.h"
#include <cstdio>

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: levelconvert FILE...\n");
		return 2;
	}
	int code = 0;
	for (int i = 1; i < argc; i++) {
		LevelGrid grid;
		int version = 0;
		std::string error = loadLevelFile(argv[i], grid, &version);
		if (!error.empty()) {
			fprintf(stderr, "%s: %s\n", argv[i], error.c_str());
			code = 2;
			continue;
		}
		if (version == LEVEL_VERSION) {
			printf("%s: already v%d\n", argv[i], LEVEL_VERSION);
			continue;
		}
		if (!saveLevelFile(argv[i], grid)) {
			fprintf(stderr, "%s: cannot write\n", argv[i]);
			code = 2;
			continue;
		}
		printf("%s: v%d -> v%d\n", argv[i], version, LEVEL_VERSION);
	}
	return code;
}
