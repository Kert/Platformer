#ifndef _gamelogic_h_
#define _gamelogic_h_

#include "globals.h"

void GameLogic();
void ResetLogic();
void AddTime();
void OnLevelExit();
void GameOver(GAME_OVER_REASONS reason);
void SetGamestate(int state);
void ChangeGamestate(int state);
void DoFading();

#endif
