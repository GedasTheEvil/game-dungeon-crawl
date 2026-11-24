// The credits' second page (plan credits-second-section): the groups and rows of content/credits.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/level/ui/credits_text.h"
#include <sstream>

TEST_CASE("credits text: headings start groups, rows split at the bar") {
	std::istringstream in("// note\n# Music\nTrack | Author | CC0\n\n# Agents\nClaude | Anthropic\n");
	const auto groups = ParseCredits(in);
	REQUIRE(groups.size() == 2);
	CHECK(groups[0].heading == "Music");
	REQUIRE(groups[0].lines.size() == 1);
	CHECK(groups[0].lines[0].fields == std::vector<std::string>{"Track", "Author", "CC0"});
	CHECK(groups[1].heading == "Agents");
	CHECK(groups[1].lines[0].fields.size() == 2);
}

TEST_CASE("credits text: the shipped file lists music, fonts and agents") {
	const auto groups = LoadCredits(CONTENT_DIR "credits");
	REQUIRE(groups.size() == 3);
	CHECK(groups[0].heading == "Music");
	CHECK(groups[0].lines.size() == 13);
	CHECK(groups[1].heading == "Fonts");
	CHECK(groups[2].heading == "Agents");
	CHECK(LoadCredits("content/no-such-credits").empty());
}
