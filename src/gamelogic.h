#ifndef _gamelogic_h_
#define _gamelogic_h_

#include <SDL.h>
#include "globals.h"

void LogicUpdate(Uint32 dt);
void ResetLogic();
void AddTime();
void OnLevelExit();
void GameOver(GAME_OVER_REASONS reason);
void SetGamestate(int state);
void ChangeGamestate(int state);
void FadingUpdate();

#endif
