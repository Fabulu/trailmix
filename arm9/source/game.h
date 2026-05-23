#ifndef GAME_H
#define GAME_H

enum class GameState : int {
    Menu     = 0,
    Play     = 1,
    Upgrade  = 2,
    GameOver = 3,
    COUNT
};

void gameInit();
void gameUpdate();
void gameRender();
GameState gameGetState();

#endif // GAME_H
