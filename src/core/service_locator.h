#ifndef SERVICE_LOCATOR_H
#define SERVICE_LOCATOR_H

#include <cassert>
#include <memory>
#include "../state/cashe.h"

class ServiceLocator {
  private:
	static std::unique_ptr<Cashe> gameState;

  public:
	static void initialize(std::unique_ptr<Cashe> state) { gameState = std::move(state); }

	static Cashe& getGameState() {
		assert(gameState && "ServiceLocator not initialized");
		return *gameState;
	}

	static void shutdown() { gameState.reset(); }
};

#define GAME_STATE ServiceLocator::getGameState()

#endif // SERVICE_LOCATOR_H
