#include "gamelogic.h"
#include <SDL.h>
#include <vector>
#include "camera.h"
#include "entities.h"
#include "graphics.h"
#include "interface.h"
#include "level.h"
#include "levelspecific.h"
#include "menu.h"
#include "physics.h"
#include "sound.h"
#include "transition.h"
#include "utils.h"

int min, sec;
int timeLimit = 300;
Timer gameTimer{ 1000 };

int playerLives;
int currentLives;
extern int SelectedItem;
extern Player *player;
extern int GameState;
int FadingState = FADING_STATE_NONE;
int FadingVal;
int FadingStart;
int FadingEnd;
unsigned FadingSpeed;
GAMESTATES toGameState;

bool menuLoaded = false;
bool gameLoaded = false;
extern int TransitionID;
// used to send it to transition
GAME_OVER_REASONS gameOverReason;

extern Level *level;

void StartGame()
{
	gameTimer.completed = false;
	gameTimer.oldTicks = SDL_GetTicks();

	min = timeLimit / 60;
	sec = timeLimit % 60;
	PlayMusic("1");
}

void GameLogic()
{
	if(player->status == STATUS_DYING)
		GameOver(GAME_OVER_REASON_DIED);

	gameTimer.Run();
	if(gameTimer.completed)
	{
		PrintLog(LOG_DEBUG, "%2d:%2d", min, sec);
		AddTime();
	}

	LevelLogic();
}

void ResetLogic()
{
	timeLimit = 300;
	sec = 0;
	min = 0;
}


void AddTime()
{
	sec--;
	if(sec < 0)
	{
		min--;
		sec = 59;
	}
	if(min < 0)
	{
		PrintLog(LOG_INFO, "Time up!");
		GameOver(GAME_OVER_REASON_TIME);
	}
	const char *c = "";
	if(sec % 2 == 0)
		c = InfoFormat(min, sec);
	//else
	//	c = InfoFormat("LOLI");

	//PrintToInterface(c, INTERFACE_TIME);
}

void OnLevelExit()
{
	PrintLog(LOG_INFO, "Exited the level!");
	GameOver(GAME_OVER_REASON_WIN);
}

void GameOver(GAME_OVER_REASONS reason)
{
	gameOverReason = reason;
	StopMusic();
	if(reason == GAME_OVER_REASON_WIN)
	{
		SetCurrentTransition(TRANSITION_LEVELCLEAR);
		ChangeGamestate(STATE_TRANSITION);
	}
	else
	{
		PlaySound("game_over");
		SetCurrentTransition(TRANSITION_LEVELLOSE);
		ChangeGamestate(STATE_TRANSITION);
	}
}

void RemoveFading()
{
	FadingState = FADING_STATE_NONE;
}

void DoFading()
{
	if(FadingState == FADING_STATE_IN)
	{
		FadingVal += FadingSpeed;
		if(FadingVal >= FadingEnd)
			FadingVal = FadingEnd;
	}
	else if(FadingState == FADING_STATE_OUT)
	{
		FadingVal -= FadingSpeed;
		if(FadingVal <= FadingEnd)
		{
			FadingVal = FadingEnd;
			RemoveFading();
		}
	}
	else if(FadingState == FADING_STATE_BLACKNBACK)
	{
		if(FadingVal <= FadingEnd)
		{
			FadingVal += FadingSpeed;
			if(FadingVal > FadingEnd)
			{
				FadingVal = 255;
				FadingEnd = 0;
				FadingState = FADING_STATE_OUT;
				SetGamestate(toGameState);
			}
		}
	}
}

void InitFading(FADING_STATES state, int start, int end, int speed)
{
	if(state == FADING_STATE_IN && start > end)
	{
		PrintLog(LOG_IMPORTANT, "Wrong fading parameters");
		return;
	}
	if(state == FADING_STATE_OUT && end > start)
	{
		PrintLog(LOG_IMPORTANT, "Wrong fading parameters");
		return;
	}
	FadingState = state;
	FadingVal = start;
	FadingStart = start;
	FadingEnd = end;
	FadingSpeed = speed;
	if(state == FADING_STATE_BLACKNBACK)
	{
		FadingStart = 0;
		FadingEnd = 255;
		FadingVal = FadingStart;
	}
}

void InitFading(FADING_STATES state, int start, int end, int speed, GAMESTATES to)
{
	InitFading(state, start, end, speed);
	toGameState = to;
}

void SetGamestate(int state)
{
	if(state == STATE_MENU)
		if(MenuRenderSetup()) gameLoaded = false;
	if(state == STATE_GAME)
		if(GameRenderSetup()) menuLoaded = false;
	if(state == STATE_TRANSITION)
	{
		MenuRenderSetup();
		if(TransitionID == TRANSITION_LEVELLOSE)
		{
			StopMusic();
			currentLives -= 1;
			if(currentLives < 1)
				SetCurrentMenu(MENU_PLAYER_FAILED_NO_ESCAPE);
			else
				SetCurrentMenu(MENU_PLAYER_FAILED);
		}
		else if(TransitionID == TRANSITION_LEVELCLEAR)
		{
			PlaySound("level_clear");
			UpdateTransition(); // don't make player wait for the level to unload to see his astonishing victory
			delete level;
			level = nullptr;
		}
	}
	if(state == STATE_PAUSED)
	{
		SetCurrentMenu(MENU_PAUSE);
		PauseMusic();
	}
	GameState = state;
}

void ChangeGamestate(int state)
{
	int oldstate = GameState;

	if(oldstate == STATE_PAUSED && state != STATE_GAME)
		RemoveFading();

	if(oldstate == STATE_GAME && state == STATE_PAUSED)
	{
		InitFading(FADING_STATE_IN, 0, 150, 5);
		SetGamestate(STATE_PAUSED);
	}
	else if(oldstate == STATE_PAUSED && state == STATE_GAME)
	{
		InitFading(FADING_STATE_OUT, 150, 0, 8);
		SetGamestate(STATE_GAME);
	}
	else if(oldstate == STATE_TRANSITION && state == STATE_GAME)
	{
		StartGame();
		InitFading(FADING_STATE_BLACKNBACK, 150, 0, 3, STATE_GAME);
	}
	else if(oldstate == STATE_GAME && state == STATE_TRANSITION)
	{
		InitFading(FADING_STATE_BLACKNBACK, 150, 0, 3, STATE_TRANSITION);
	}
	else
		SetGamestate(state);
}