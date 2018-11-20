#include "input.h"
#include <map>
#include "entities.h"
#include "gamelogic.h"
#include "globals.h"
#include "level.h"
#include "menu.h"
#include "physics.h"
#include "sound.h"
#include "transition.h"
#include "utils.h"

extern Player *player;
extern std::vector<Machinery*> machinery;
extern int GameState;
extern int SelectedItem;
extern bool loadDebugStuff;
extern int FadingState;
extern int TransitionID;

std::map<SDL_Keycode, int> mkeys;
std::map<Uint8, int> jbuttons;

//Game Controller 1 handler
SDL_Joystick* gamepad = NULL;

// ends gameloop
extern bool ENDGAME;

extern Level *level;

enum
{
	KEYSTATE_UNPRESSED,
	KEYSTATE_PRESSED
};

void InitInput()
{
	// Check for joysticks
	if(SDL_NumJoysticks() < 1)
	{
		PrintLog(LOG_INFO, "Warning: No joysticks connected!");
	}
	else
	{
		// Load joystick
		// For now use only 1 joystick, first found
		gamepad = SDL_JoystickOpen(0);
		if(gamepad == NULL)
		{
			PrintLog(LOG_IMPORTANT, "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
		}
	}
}

bool IsBindPressed(int bind)
{
	int code = GetBindingCode(bind);
	if(mkeys[code])
		return true;
	return false;
}

bool OnBindPress(int bind)
{
	if(FadingState == FADING_STATE_BLACKNBACK)
		return true;
	if(GameState == STATE_GAME)
	{
		player->HandleInput(bind, 0);

		// end inputs affected by player state
		switch(bind)
		{
			case BIND_RIGHT:
				break;
			case BIND_LEFT:
				break;
			case BIND_JUMP:
				break;
			case BIND_UP:
				break;
			case BIND_DOWN:
				break;
			case BIND_BACK: case BIND_ESCAPE:
				ChangeGamestate(STATE_PAUSED);
				break;
		}
	}
	else if(GameState == STATE_PAUSED)
	{
		switch(bind)
		{
			case BIND_RIGHT: case BIND_ARROWR:
				break;
			case BIND_LEFT: case BIND_ARROWL:
				break;
			case BIND_UP: case BIND_DOWN: case BIND_ARROWUP: case BIND_ARROWDOWN:
				NavigateMenu(bind);
				break;
			case BIND_JUMP: case BIND_OK: case BIND_ENTER:
				if(SelectedItem == 0)
				{
					ChangeGamestate(STATE_GAME);
					ResumeMusic();
				}
				else if(SelectedItem == 1)
				{
					delete level;
					level = nullptr;
					ChangeGamestate(STATE_MENU);
					SetCurrentMenu(MENU_MAIN);
					StopMusic();
				}
				break;
		}
	}
	else if(GameState == STATE_MENU)
	{
		return false;
	}
	else if(GameState == STATE_TRANSITION)
	{
		switch(bind)
		{
			case BIND_RIGHT:
				break;
			case BIND_LEFT:
				break;
			case BIND_UP: case BIND_DOWN:
				NavigateMenu(bind);
				break;
			case BIND_JUMP: case BIND_OK: case BIND_ENTER:
				ProgressTransition();
				break;
		}
	}
	return true;
}

void OnBindUnpress(int bind)
{
	if(GameState == STATE_GAME)
	{
		player->HandleInput(bind, 2);
	}
}

void OnBindHold(int bind)
{
	if(GameState == STATE_GAME)
	{
		player->HandleInput(bind, 1);
	}
}

void OnHardcodedKeyPress(SDL_Keycode key)
{
	if(FadingState == FADING_STATE_BLACKNBACK)
		return;
	int bind = GetBindingFromCode(key);

	if(GameState == STATE_GAME)
	{
		// debug!!
		if(loadDebugStuff || mkeys[SDLK_LSHIFT] || mkeys[SDLK_RSHIFT])
		{
			if(key == SDLK_KP_4 || key == SDLK_j)
			{
				int x, y;
				player->GetPos(x, y);
				player->SetPos(x - 32, y);
			}
			else if(key == SDLK_KP_6 || key == SDLK_l)
			{
				int x, y;
				player->GetPos(x, y);
				player->SetPos(x + 32, y);
			}
			else if(key == SDLK_KP_2 || key == SDLK_k)
			{
				int x, y;
				player->GetPos(x, y);
				player->SetPos(x, y + 32);
			}
			else if(key == SDLK_KP_8 || key == SDLK_i)
			{
				int x, y;
				player->GetPos(x, y);
				player->SetPos(x, y - 32);
			}
			else if(key == SDLK_7)
			{
				if(player->status != STATUS_INVULN)
				{
					player->status = STATUS_INVULN;
					player->statusTimer = INT_MAX;
				}
				else
				{
					player->status = STATUS_NORMAL;
					player->statusTimer = 0;
				}
			}
			else if(key == SDLK_8)
			{
				int x, y;
				player->GetPos(x, y);

				Creature *a = new Creature("Dale");
				a->SetPos(x + 50, y);
			}
			else if(key == SDLK_9)
				TestMemory();
			else if(key == SDLK_0)
				level->Reload();
			else if(key == SDLK_HOME)
			{
				player->GiveWeapon(WEAPON_FLAME);
				player->ammo[WEAPON_FLAME] += 50;
				player->GiveWeapon(WEAPON_GRENADE);
				player->ammo[WEAPON_GRENADE] += 50;
				player->GiveWeapon(WEAPON_ROCKETL);
				player->ammo[WEAPON_ROCKETL] += 50;
			}
		}
	}
	else if(GameState == STATE_MENU)
	{
		DoMenuAction(key, bind);
	}
	else if(GameState == STATE_TRANSITION)
	{
		ProgressTransition();
	}
}

int GetCodeFromInputEvent(SDL_Keycode key, Uint8 jbutton)
{
	int code;
	if(key != -1)
	{
		// Enter is a default hardcoded binding, but its code matches a joy button
		// This makes sure there are no conflicts relating to that
		if(key == SDLK_RETURN) key = SDLK_RETURN2;

		code = key;
	}
	else code = jbutton;
	return code;
}

void OnKeyPress(SDL_Keycode key, Uint8 jbutton)
{
	PrintLog(LOG_SUPERDEBUG, "Pressed key %d jbutton %u", key, jbutton);
	bool inputHandled = false;
	int code = GetCodeFromInputEvent(key, jbutton);
	int bind = GetBindingFromCode(code);

	if(bind != -1)
		inputHandled = OnBindPress(bind);

	//inputEvents.push_back(bind, KEYSTATE_PRESSED);

	if(!inputHandled)
		OnHardcodedKeyPress(code);
}

void OnKeyUnpress(SDL_Keycode key, Uint8 jbutton)
{
	PrintLog(LOG_SUPERDEBUG, "Unpressed key %d jbutton %u", key, jbutton);
	int code, bind;
	code = GetCodeFromInputEvent(key, jbutton);
	bind = GetBindingFromCode(code);

	//inputEvents.push_back(bind, KEYSTATE_UNPRESSED);

	OnBindUnpress(bind);
}

void OnKeyHold(SDL_Keycode key, Uint8 jbutton)
{
	int code, bind;
	code = GetCodeFromInputEvent(key, jbutton);
	bind = GetBindingFromCode(code);
	OnBindHold(bind);
}

void ProcessInput()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(!e.key.repeat)
		{
			switch(e.type)
			{
				case SDL_EventType::SDL_KEYDOWN:
					mkeys[e.key.keysym.sym] = KEYSTATE_PRESSED;
					OnKeyPress(e.key.keysym.sym, 255);
					break;
				case SDL_EventType::SDL_KEYUP:
					mkeys[e.key.keysym.sym] = KEYSTATE_UNPRESSED;
					OnKeyUnpress(e.key.keysym.sym, 255);
					break;
			}
		}
		switch(e.type)
		{
			case SDL_EventType::SDL_JOYBUTTONDOWN:
				jbuttons[e.jbutton.button] = KEYSTATE_PRESSED;
				OnKeyPress(-1, e.jbutton.button);
				break;
			case SDL_EventType::SDL_JOYBUTTONUP:
				jbuttons[e.jbutton.button] = KEYSTATE_UNPRESSED;
				OnKeyUnpress(-1, e.jbutton.button);
				break;
		}
		if(e.window.event == SDL_WINDOWEVENT_CLOSE)
			ENDGAME = true;
	}
	for(auto key : mkeys)
	{
		if(key.second == KEYSTATE_PRESSED)
			OnKeyHold(key.first, 255);
	}
	for(auto jbutton : jbuttons)
	{
		if(jbutton.second == KEYSTATE_PRESSED)
			OnKeyHold(-1, jbutton.first);
	}
}

void InputCleanup()
{
	//Close game controller
	SDL_JoystickClose(gamepad);
	gamepad = NULL;

	// forcibly deallocate memory
	std::map<SDL_Keycode, int>().swap(mkeys);
	std::map<Uint8, int>().swap(jbuttons);
}
