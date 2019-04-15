#ifndef _gamelogic_h_
#define _gamelogic_h_

#include <SDL.h>
#include "entities.h"
#include "globals.h"
#include "level.h"

namespace Game
{
	Player* GetPlayer();
	Player* CreatePlayer();
	void RemovePlayer();
	Level* GetLevel();
	void CreateLevel(std::string fileName);
	void RemoveLevel();
	void CheckDebugMode();
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
	bool IsGameEndRequested();
	void SetGameEndFlag();
	bool IsDebug();
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
