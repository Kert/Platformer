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
extern bool IsDebugMode;
extern int FadingState;

std::map<KEYBINDS, bool> kb_keys;
std::map<KEYBINDS, bool> j_buttons;

SDL_GameController *controller = nullptr;

// ends gameloop
extern bool GameEndFlag;

extern Level *level;

enum
{
	KEYSTATE_UNPRESSED,
	KEYSTATE_PRESSED
};

void InitInput()
{
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
			break;
		}		
	}
	if(!controller)
	{
		PrintLog(LOG_INFO, "Warning: No joysticks connected!");
		return;
	}
}

bool IsBindPressed(KEYBINDS bind)
{
	if(kb_keys[bind] || j_buttons[bind])
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
		return false;
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

void OnHardcodedKeyPress(SDL_Keycode key, Uint8 jbutton)
{
	if(FadingState == FADING_STATE_BLACKNBACK)
		return;
	int bind;
	if(jbutton != 255)
		bind = GetControllerBindFromCode(jbutton);
	else
		bind = GetKeyboardBindFromCode(key);

	if(GameState == STATE_GAME)
	{
		// debug!!
		/*if(IsDebugMode || kb_keys[SDLK_LSHIFT] || kb_keys[SDLK_RSHIFT])
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
		}*/
	}
	else if(GameState == STATE_MENU || GameState == STATE_PAUSED)
	{
		DoMenuAction(key, jbutton, bind);
	}
	else if(GameState == STATE_TRANSITION)
	{
		ProgressTransition();
	}
}

void OnKeyPress(SDL_Keycode key, Uint8 jbutton)
{
	PrintLog(LOG_SUPERDEBUG, "Pressed key %d jbutton %u", key, jbutton);
	
	int bind;
	if(jbutton != 255)
		bind = GetControllerBindFromCode(jbutton);
	else
		bind = GetKeyboardBindFromCode(key);

	bool inputHandled = false;
	if(bind != -1)
	{
		inputHandled = OnBindPress(bind);
		if(jbutton != 255)
			j_buttons[(KEYBINDS)bind] = KEYSTATE_PRESSED;
		else
			kb_keys[(KEYBINDS)bind] = KEYSTATE_PRESSED;
	}
	
	//inputEvents.push_back(bind, KEYSTATE_PRESSED);

	if (!inputHandled)
		OnHardcodedKeyPress(key, jbutton);
}

void OnKeyUnpress(SDL_Keycode key, Uint8 jbutton)
{
	PrintLog(LOG_SUPERDEBUG, "Unpressed key %d jbutton %u", key, jbutton);
	int bind;
	if(jbutton != 255)
		bind = GetControllerBindFromCode(jbutton);
	else
		bind = GetKeyboardBindFromCode(key);

	if(jbutton != 255)
		j_buttons[(KEYBINDS)bind] = KEYSTATE_UNPRESSED;
	else
		kb_keys[(KEYBINDS)bind] = KEYSTATE_UNPRESSED;

	//inputEvents.push_back(bind, KEYSTATE_UNPRESSED);

	OnBindUnpress(bind);
}

void OnKeyHold(SDL_Keycode key, Uint8 jbutton)
{
	int bind;
	if(jbutton != 255)
		bind = GetControllerBindFromCode(jbutton);
	else
		bind = GetKeyboardBindFromCode(key);
	OnBindHold(bind);
}

void InputUpdate()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		if(!e.key.repeat || (e.key.repeat && GameState == GAMESTATES::STATE_MENU))
		{
			switch(e.type)
			{
				case SDL_EventType::SDL_KEYDOWN:
					OnKeyPress(e.key.keysym.sym, 255);
					break;
				case SDL_EventType::SDL_KEYUP:
					OnKeyUnpress(e.key.keysym.sym, 255);
					break;
			}
		}
		switch(e.type)
		{
			case SDL_CONTROLLERBUTTONDOWN:
				OnKeyPress(-1, e.cbutton.button);
				break;
			case SDL_CONTROLLERBUTTONUP:
				OnKeyUnpress(-1, e.cbutton.button);
				break;
		}
		if(e.type == SDL_WINDOWEVENT)
		{
			if(e.window.event == SDL_WINDOWEVENT_CLOSE)
				GameEndFlag = true;
		}
	}
	for(auto key : kb_keys)
	{
		if(key.second == KEYSTATE_PRESSED)
			OnBindHold(key.first);
	}
	for(auto jbutton : j_buttons)
	{
		if(jbutton.second == KEYSTATE_PRESSED)
			OnBindHold(jbutton.first);
	}
}

void InputCleanup()
{
	SDL_GameControllerClose(controller);
	controller = NULL;

	// forcibly deallocate memory
	std::map<KEYBINDS, bool>().swap(kb_keys);
	std::map<KEYBINDS, bool>().swap(j_buttons);
}
