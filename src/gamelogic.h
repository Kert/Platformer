#ifndef _gamelogic_h_
#define _gamelogic_h_

#include <SDL.h>
#include "globals.h"

namespace Game
{
	GAMESTATES GetState();
	void SetState(GAMESTATES state);
	void ChangeState(GAMESTATES state);
	void Start();
	GAME_OVER_REASONS GetGameOverReason();
	void GameOver(GAME_OVER_REASONS reason);
	void Update(Uint32 dt);
	void Reset();
	void AddTime();
	void OnLevelExit();
	int GetTimeLimit();
	int GetPlayerLivesLeft();
	void ResetPlayerLives();
}

namespace Fading
{
	int GetVal();
	FADING_STATES GetState();
	void Update();
	void Remove();
	void Init(FADING_STATES state, int start, int end, int speed);
	void Init(FADING_STATES state, int start, int end, int speed, GAMESTATES to);
}

#endif
