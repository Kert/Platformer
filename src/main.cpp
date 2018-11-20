#include "main.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <chrono>
#include <fstream>
#include <thread>
//#include <vld.h>
#include "camera.h"
#include "config.h"
#include "entities.h"
#include "gamelogic.h"
#include "graphics.h"
#include "input.h"
#include "interface.h"
#include "level.h"
#include "menu.h"
#include "physics.h"
#include "sound.h"
#include "tiles.h"
#include "transition.h"
#include "utils.h"

using namespace std::chrono_literals;
constexpr std::chrono::nanoseconds timestep(8ms);
constexpr std::chrono::nanoseconds timestepGraph(7ms);

bool ENDGAME = false;
bool loadDebugStuff; // loads if "debugme" file exists
Player *player;
RandomGenerator *rg;
int GameState;
extern int FadingState;
extern std::vector<Bullet*> bullets;
extern std::vector<Effect*> effects;
extern std::vector<Creature*> creatures;
extern std::vector<Pickup*> pickups;
extern std::vector<Machinery*> machinery;
extern std::vector<Lightning*> lightnings;

extern Level *level;
extern Camera* camera;

template<typename T>
void CleanFromNullPointers(std::vector<T> *collection)
{
	for(auto j = collection->begin(); j != collection->end();)
	{
		if(*j == nullptr)
			j = collection->erase(j);
		else
			++j;
	}
}

void CallUpdate(Uint32 dt)
{
	player->HandleStateIdle();
	ApplyPhysics(*player, dt);
	// Updating camera
	if(player->hasState(STATE_ONLADDER))
	{
		if(player->GetVelocity().y > 0)
			camera->SetOffsetY(30);
		else if(player->GetVelocity().y < 0)
			camera->SetOffsetY(-10);
		else
			camera->SetOffsetY(10);
	}
	if(player->hasState(STATE_LOOKINGUP))
		camera->SetOffsetY(-20);
	else if(!player->hasState(STATE_ONLADDER))
		camera->SetOffsetY(15);
	if(player->hasState(STATE_DUCKING))
		camera->SetOffsetY(35);
	camera->Update();

	for(auto &m : machinery)
	{
		ApplyPhysics(*m, dt);
	}

	for(auto &b : bullets)
	{
		ApplyPhysics(*b, dt);
	}
	CleanFromNullPointers(&bullets);
	CleanFromNullPointers(&creatures); // they can be dead already

	for(auto &l : lightnings)
	{
		if(!ApplyPhysics(*l, dt))
			break; // workaround to stop physicsing once a lightning is deleted
	}
	CleanFromNullPointers(&creatures); // they can be dead already

	for(auto &i : creatures)
	{
		if(player->hitbox->HasCollision(i->hitbox))
		{
			OnHitboxCollision(*player, *i, dt);
			PrintLog(LOG_SUPERDEBUG, "what %d", SDL_GetTicks());
		}
		if(i->AI)
			i->AI->RunAI();
		ApplyPhysics(*i, dt);
		UpdateStatus(*i, 8);
		if(i->REMOVE_ME)
			delete i;
	}
	CleanFromNullPointers(&creatures);

	for(auto &j : pickups)
	{
		if(player->hitbox->HasCollision(j->hitbox))
		{
			OnHitboxCollision(*player, *j, dt);
			PrintLog(LOG_SUPERDEBUG, "what %d", SDL_GetTicks());
		}
		if(j->status == STATUS_DYING)
		{
			if(!UpdateStatus(*j, dt))
				break; // j has been deleted, let's get out of here
		}
	}
	for(auto &e : effects)
	{
		if(e->status == STATUS_DYING)
		{
			if(!UpdateStatus(*e, dt))
				break; // j has been deleted, let's get out of here
		}
	}
}

void CallDraw()
{
	GraphicsUpdate();
}

void MenuLogic()
{
	UpdateMenu();
}

void Cleanup()
{
	// let each file handle their own disposing (avoids giant bulky function)
	try
	{
		GraphicsCleanup();
		if(level != nullptr)
			delete level;
		EntityCleanup();
		InputCleanup();
		BindingsCleanup();
		TilesCleanup();
		InterfaceCleanup();
		MenusCleanup();
		SoundCleanup();

		// remove random generator
		delete rg;
	}
	catch(...) {};
	// close SDL
	SDL_Quit();
}

bool CheckDebugConfig()
{
	std::ifstream infile("debugme");
	return infile.good();
}

int main(int argc, char* argv[])
{
	loadDebugStuff = CheckDebugConfig();
	//VLDEnable();
	// Initialize SDL.
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
		return 1;

	// Initialize audio
	Mix_Init(MIX_INIT_OGG);
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 512) == -1)
	{
		return false;
	}
	Mix_Volume(-1, 50);

	// Initialize RandomGenerator
	rg = new RandomGenerator();
	rg->ExportSeed();

	// Initialize gamepad support
	InitInput();

	// Initialize config settings
	InitConfig();

	// Loading textures and tilesets
	GraphicsSetup();

	// Load menus
	LoadMenus();

	// Load creature properties and their sprite data
	ReadCreatureData();

	// Setting game state
	SetCurrentTransition(TRANSITION_TITLE);
	SetGamestate(STATE_TRANSITION);

	// used for fps limiting
	Uint32 upd_graph = SDL_GetTicks();
	// Fixed time step game loop wizardry
	using clock = std::chrono::high_resolution_clock;
	std::chrono::nanoseconds lag(0ns);
	std::chrono::nanoseconds lagGraph(0ns);
	auto time_start = clock::now();
	auto time_start2 = clock::now();
	// main loop
	int cntr = 0;
	while(!ENDGAME)
	{
		auto delta_time = clock::now() - time_start;
		time_start = clock::now();
		lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

		ProcessInput();

		while(lag >= timestep)
		{
			lag -= timestep;

			//previous_state = current_state;
			//update(&current_state); // update at a fixed rate each time
			if(FadingState != FADING_STATE_NONE)
				DoFading();
			if(GameState == STATE_MENU)
				MenuLogic();
			else if(GameState == STATE_GAME && FadingState != FADING_STATE_BLACKNBACK)
			{
				GameLogic();
				if(GameState == STATE_GAME) // we may have died or won, don't keep processing if so
					CallUpdate(8);
				cntr++;
			}
			else if(GameState == STATE_PAUSED)
			{
				//ShowPauseOverlay();
			}
			else if(GameState == STATE_TRANSITION)
			{
				//UpdateTransition();
			}
		}

		// calculate how close or far we are from the next timestep
		auto alpha = (float)lag.count() / timestep.count();
		//auto interpolated_state = interpolate(current_state, previous_state, alpha);
		//render(interpolated_state);

		auto delta_time2 = clock::now() - time_start2;

		Uint32 dtG = SDL_GetTicks() - upd_graph;
		if(delta_time2 >= timestepGraph) // 125fps draw ONE frame
		{
			FlushWindow();
			if(GameState == STATE_GAME || GameState == STATE_PAUSED)
			{
				CallDraw();
				if(FadingState != FADING_STATE_NONE)
				{
					DrawFading();
				}
				if(GameState == STATE_PAUSED)
				{
					ShowPauseOverlay();
				}
				//DrawFPS(dtG);
				upd_graph = SDL_GetTicks();
				UpdateWindow();
			}
			else
			{
				if(GameState == STATE_MENU)
					RenderMenu();
				if(GameState == STATE_TRANSITION)
					UpdateTransition();
				if(FadingState != FADING_STATE_NONE)
				{
					DrawFading();
				}
				UpdateWindow();
			}
			time_start2 = clock::now();
		}

		// Workaround to allow for gapless ogg looping without bugs
		ProcessMusic();
		// prevent 100% usage of cpu
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}

	// I'm not sure if this is even necessary since the program is about to quit and lose all memory anyway
	// I mostly used it to get VLD to stop yelling at me :D
	Cleanup();
	//VLDReportLeaks();

	return 0;
}
