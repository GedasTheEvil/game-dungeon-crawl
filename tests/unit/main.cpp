// The doctest runner (external/doctest). The tests are the other files in tests/unit/.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../external/doctest/doctest.h"
#include "../../src/core/timer.h"

namespace {
// Well above 0, so no "0 means unset" field mistakes a moment at the start for "never".
constexpr int START_MS = 100000;

// Every test case starts on its own virtual clock, the same whatever ran before it.
class VirtualClockPerCase : public doctest::IReporter {
  public:
	explicit VirtualClockPerCase(const doctest::ContextOptions&) {}
	void test_case_start(const doctest::TestCaseData&) override { GameClock::resetVirtual(START_MS); }
	void test_case_reenter(const doctest::TestCaseData&) override {
		GameClock::resetVirtual(START_MS);
	} // a subcase's rerun
	void report_query(const doctest::QueryData&) override {}
	void test_run_start() override {}
	void test_run_end(const doctest::TestRunStats&) override {}
	void test_case_end(const doctest::CurrentTestCaseStats&) override {}
	void test_case_exception(const doctest::TestCaseException&) override {}
	void subcase_start(const doctest::SubcaseSignature&) override {}
	void subcase_end() override {}
	void log_assert(const doctest::AssertData&) override {}
	void log_message(const doctest::MessageData&) override {}
	void test_case_skipped(const doctest::TestCaseData&) override {}
};
DOCTEST_REGISTER_LISTENER("virtual_clock_per_case", 1, VirtualClockPerCase);
} // namespace
