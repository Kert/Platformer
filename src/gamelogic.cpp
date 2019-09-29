#include "gamelogic.h"
#include <SDL.h>
#include <fstream>
#include <vector>
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

extern std::vector<Bullet*> bullets;
extern std::vector<Effect*> effects;
extern std::vector<Creature*> creatures;
extern std::vector<Pickup*> pickups;
extern std::vector<Machinery*> machinery;
extern std::vector<Lightning*> lightnings;

namespace Game
{
	bool GameEndFlag = false;
	bool Debug = false;
	GAMESTATES GameState;
	GAME_OVER_REASONS gameOverReason;
	int min, sec;
	int timeLimit = 300;
	Timer gameTimer{ 1000 };
	int playerLives = 3;
	int currentLives;
	Player *player;
	Level *level = nullptr;

	Player* GetPlayer()
	{
		return player;
	}

	Player* CreatePlayer()
	{
		player = new Player();
		return player;
	}

	void RemovePlayer()
	{
		delete player;
		player = nullptr;
	}

	Level* GetLevel()
	{
		return level;
	}

	void CreateLevel(std::string fileName)
	{
		level = new Level(fileName);
	}

	void RemoveLevel()
	{
		if(level != nullptr)
			delete level;
		level = nullptr;
	}

	void SetDebug(bool toggle)
	{
		Debug = toggle;
	}

	GAMESTATES GetState()
	{
		return GameState;
	}

	void SetState(GAMESTATES state)
	{
		if(state == STATE_TRANSITION)
		{
			if(GetCurrentTransition() == TRANSITION_LEVELCLEAR)
			{
				Sound::PlaySfx("level_clear");
				Graphics::RenderTransition(); // don't make player wait for the level to unload to see his astonishing victory
				RemoveLevel();
			}
		}
		if(state == STATE_PAUSED)
		{
			SetCurrentMenu(MENU_PAUSE);
			Sound::PauseMusic();
		}
		GameState = state;
	}

	void ChangeState(GAMESTATES state)
	{
		GAMESTATES oldstate = GameState;

		if(oldstate == STATE_PAUSED && state != STATE_GAME)
			Fading::Remove();

		if(oldstate == STATE_GAME && state == STATE_PAUSED)
		{
			Fading::Init(FADING_STATE_IN, 0, 150, 0.2);
			SetState(STATE_PAUSED);
		}
		else if(oldstate == STATE_PAUSED && state == STATE_GAME)
		{
			Fading::Init(FADING_STATE_OUT, 150, 0, 0.2);
			SetState(STATE_GAME);
		}
		else if(oldstate == STATE_TRANSITION && state == STATE_GAME)
		{
			Start();
			Fading::Init(FADING_STATE_BLACKNBACK, 150, 0, 0.3, STATE_GAME);
		}
		else if(oldstate == STATE_GAME && state == STATE_TRANSITION)
		{
			Fading::Init(FADING_STATE_BLACKNBACK, 150, 0, 0.3, STATE_TRANSITION);
		}
		else
			SetState(state);
	}

	void Start()
	{
		gameTimer.completed = false;
		gameTimer.oldTicks = SDL_GetTicks();

		min = timeLimit / 60;
		sec = timeLimit % 60;
		Sound::PlayMusic(level->musicFileName);
		Graphics::GetCamera()->Attach(*player);
	}

	GAME_OVER_REASONS GetGameOverReason()
	{
		return gameOverReason;
	}

	void GameOver(GAME_OVER_REASONS reason)
	{
		gameOverReason = reason;
		Sound::StopMusic();
		if(reason == GAME_OVER_REASON_WIN)
		{
			SetCurrentTransition(TRANSITION_LEVELCLEAR);
			ChangeState(STATE_TRANSITION);
		}
		else
		{
			Sound::PlaySfx("game_over");
			Fading::Init(FADING_STATE_BLACKNBACK, 150, 0, 0.3, STATE_MENU);
			Sound::StopMusic();
			currentLives -= 1;
			if(currentLives < 1)
				SetCurrentMenu(MENU_PLAYER_FAILED_NO_ESCAPE);
			else
				SetCurrentMenu(MENU_PLAYER_FAILED);
		}
	}

	void Update(double ticks)
	{
		if(player->status == STATUS_DYING)
		{
			GameOver(GAME_OVER_REASON_DIED);
			return;
		}

		//gameTimer.Run();
		//if(gameTimer.completed)
		//{
		//	PrintLog(LOG_DEBUG, "%2d:%2d", min, sec);
		//	AddTime();
		//}

		//LevelLogic();

		for(auto &m : machinery)
		{
			if(m->type == MACHINERY_TYPES::MACHINERY_PLATFORM)
			{
				ApplyPhysics(*(Platform*)m, ticks);
			}
		}

		player->HandleStateIdle();
		ApplyPhysics(*player, ticks);
		//// Updating camera
		//if(player->hasState(STATE_LOOKINGUP))
		//	camera->SetOffsetY(-20);
		//if(player->hasState(STATE_DUCKING))
		//	camera->SetOffsetY(35);
		Graphics::GetCamera()->Update();

		for(auto &b : bullets)
		{
			ApplyPhysics(*b, ticks);
		}
		CleanFromNullPointers(&bullets);
		CleanFromNullPointers(&creatures); // they can be dead already

		for(auto &l : lightnings)
		{
			if(!ApplyPhysics(*l, ticks))
				break; // workaround to stop physicsing once a lightning is deleted
		}
		CleanFromNullPointers(&creatures); // they can be dead already

		for(auto &i : creatures)
		{
			if(player->hitbox->HasCollision(i->hitbox))
			{
				OnHitboxCollision(*player, *i, ticks);
				PrintLog(LOG_SUPERDEBUG, "what %d", SDL_GetTicks());
			}
			if(i->AI)
				i->AI->RunAI(ticks);
			ApplyPhysics(*i, ticks);
			UpdateStatus(*i, ticks);
			if(i->REMOVE_ME)
				delete i;
		}
		CleanFromNullPointers(&creatures);

		for(auto &j : pickups)
		{
			if(player->hitbox->HasCollision(j->hitbox))
			{
				OnHitboxCollision(*player, *j, ticks);
				PrintLog(LOG_SUPERDEBUG, "what %d", SDL_GetTicks());
			}
			if(j->status == STATUS_DYING)
			{
				if(!UpdateStatus(*j, ticks))
					break; // j has been deleted, let's get out of here
			}
		}
		for(auto &e : effects)
		{
			if(e->status == STATUS_DYING)
			{
				if(!UpdateStatus(*e, ticks))
					break; // j has been deleted, let's get out of here
			}
		}
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
		std::string time = "";
		//if(sec % 2 == 0) // swap messages every 2 seconds
		time = InfoFormat(min, sec);
		//else
		//	c = InfoFormat("LOLI"); // show another message

		PrintToInterface(time, INTERFACE_TIME);
	}

	void OnLevelExit()
	{
		PrintLog(LOG_INFO, "Exited the level!");
		GameOver(GAME_OVER_REASON_WIN);
	}

	void Reset()
	{
		timeLimit = 300;
		sec = 0;
		min = 0;
	}

	int GetTimeLimit()
	{
		return timeLimit;
	}

	int GetPlayerLivesLeft()
	{
		return currentLives;
	}

	void ResetPlayerLives()
	{
		currentLives = playerLives;
	}

	bool IsGameEndRequested()
	{
		return GameEndFlag;
	}

	void SetGameEndFlag()
	{
		GameEndFlag = true;
	}

	bool IsDebug()
	{
		return Debug;
	}
}

namespace Fading
{
	FADING_STATES FadingState = FADING_STATE_NONE;
	double FadingVal;
	int FadingStart;
	int FadingEnd;
	double FadingSpeed; // alpha channel change per frame
	GAMESTATES toGameState;
	
	int GetVal()
	{
		return (int)FadingVal;
	}

	FADING_STATES GetState()
	{
		return FadingState;
	}

	void Remove()
	{
		FadingState = FADING_STATE_NONE;
	}

	void Update(double ticks)
	{
		double change = FadingSpeed * ticks;
		if(FadingState == FADING_STATE_IN)
		{
			FadingVal += change;
			if(FadingVal >= FadingEnd)
				FadingVal = FadingEnd;
		}
		else if(FadingState == FADING_STATE_OUT)
		{
			FadingVal -= change;
			if(FadingVal <= FadingEnd)
			{
				FadingVal = FadingEnd;
				Remove();
			}
		}
		else if(FadingState == FADING_STATE_BLACKNBACK)
		{
			if(FadingVal <= FadingEnd)
			{
				FadingVal += change;
				if(FadingVal > FadingEnd)
				{
					FadingVal = 255;
					FadingEnd = 0;
					FadingState = FADING_STATE_OUT;
					Game::SetState(toGameState);
				}
			}
		}
	}

	void Init(FADING_STATES state, int start, int end, double sec)
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
		FadingSpeed = abs(FadingStart - FadingEnd) / SecToTicks(sec);
		if(state == FADING_STATE_BLACKNBACK)
		{
			FadingStart = 0;
			FadingEnd = 255;
			FadingVal = FadingStart;
		}
	}

	void Init(FADING_STATES state, int start, int end, double sec, GAMESTATES to)
	{
		Init(state, start, end, sec);
		toGameState = to;
	}
}