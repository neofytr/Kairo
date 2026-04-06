#include "core/engine.h"
#include "game.h"

int main() {
    kairo::Engine engine;

    if (!engine.init({
        { "KAIRO: Dungeon", 1280, 720, true }
    })) {
        return 1;
    }

    DungeonGame game;
    engine.run(game);
    engine.shutdown();

    return 0;
}
