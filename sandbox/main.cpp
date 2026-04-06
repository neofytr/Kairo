#include "core/engine.h"
#include "game.h"

int main() {
    kairo::Engine engine;

    if (!engine.init({
        { "Kairo Engine", 1280, 720, true }
    })) {
        return 1;
    }

    Game game;
    engine.run(game);
    engine.shutdown();

    return 0;
}
