#ifndef SERVICE_LOCATOR_H
#define SERVICE_LOCATOR_H

#include <memory>
#include <stdexcept>
#include "../state/cashe.h"

class ServiceLocator {
  private:
	static std::unique_ptr<Cashe> gameState;
	static Cashe* nullState;

  public:
	static void initialize(std::unique_ptr<Cashe> state) {
		gameState = std::move(state);
		// Set global pointer for backward compatibility
		extern Cashe* c;
		c = gameState.get();
	}

	static Cashe& getGameState() {
		if (gameState) {
			return *gameState;
		}
		// Return null state during early initialization to prevent crashes
		if (!nullState) {
			nullState = new Cashe();
		}
		return *nullState;
	}

	static void shutdown() {
		extern Cashe* c;
		c = nullptr;
		gameState.reset();
		delete nullState;
		nullState = nullptr;
	}
};

// Global access macro for easier migration
#define GAME_STATE ServiceLocator::getGameState()

#endif // SERVICE_LOCATOR_H
