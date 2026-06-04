#include "service_locator.h"

std::unique_ptr<Cashe> ServiceLocator::gameState;
Cashe* ServiceLocator::nullState = nullptr;
