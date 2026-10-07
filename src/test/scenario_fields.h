#ifndef SCENARIO_FIELDS_H
#define SCENARIO_FIELDS_H

// What `expect` reads from the running game (the fields: scenario_script.h).

#include "scenario_script.h"

namespace Scenario {
// The value of cmd's field (cmd.item for the item counts) now.
[[nodiscard]] float fieldValue(const Command& cmd);
} // namespace Scenario

#endif
