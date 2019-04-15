#include "main.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <chrono>
#include <fstream>
#include <thread>
//#include <vld.h>
#include "config.h"
#include "gamelogic.h"
#include "graphics.h"
#include "input.h"
#include "interface.h"
#include "level.h"
#include "menu.h"
#include "sound.h"
#include "tiles.h"
#include "transition.h"

bool GameEndFlag = false;
bool IsDebugMode; // loads if "debugme" file exists
int GameState;
extern int FadingState;

bool CheckDebugConfig();
void Cleanup();

int main(int argc, char* argv[])
{
	IsDebugMode = CheckDebugConfig();
	//VLDEnable();
	// Initialize SDL.
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
		return 1;

	// Initialize gamepad support
	InitInput();

	// Music and SFX support
	Sound::Init();

	// Initialize config settings
	InitConfig();
	
	// Loading textures and tilesets
	Graphics::Init();

	// Load creature properties and their sprite data
	ReadCreatureData();
	// TODO: Combine this somehow
	ReadPlatformData();

	// Setting game state
	SetCurrentTransition(TRANSITION_TITLE);
	SetGamestate(STATE_TRANSITION);

	// Fixed time step game loop wizardry
	using namespace std::chrono_literals;
	constexpr std::chrono::nanoseconds timestep(8ms);
	constexpr std::chrono::nanoseconds timestepGraphics(7ms);
	using clock = std::chrono::high_resolution_clock;
	std::chrono::nanoseconds lag_logic(0ns);
	std::chrono::nanoseconds lagGraph(0ns);
	auto timeStart_logic = clock::now();
	auto timeStart_graphics = clock::now();
	// main loop
	while(!GameEndFlag)
	{
		auto deltaTime_logic = clock::now() - timeStart_logic;
		timeStart_logic = clock::now();
		lag_logic += std::chrono::duration_cast<std::chrono::nanoseconds>(deltaTime_logic);

		InputUpdate();

		while(lag_logic >= timestep)
		{
			//previous_state = current_state;
			//update(&current_state); // update at a fixed rate each time
			lag_logic -= timestep;
			if(FadingState != FADING_STATE_NONE)
				FadingUpdate();
			if(GameState == STATE_GAME && FadingState != FADING_STATE_BLACKNBACK)
			{
				if(GameState != STATE_MENU)
					LogicUpdate(8);
			}
		}

		// calculate how close or far we are from the next timestep
		auto alpha = (float)lag_logic.count() / timestep.count();
		//auto interpolated_state = interpolate(current_state, previous_state, alpha);
		//render(interpolated_state);

		auto deltaTime_graphics = clock::now() - timeStart_graphics;
		if(deltaTime_graphics >= timestepGraphics) // 125fps draw ONE frame
		{
			Graphics::WindowFlush();
			if(GameState == STATE_GAME || GameState == STATE_PAUSED)
				Graphics::Update();
			else
			{
				if(GameState == STATE_MENU)
					Graphics::RenderMenu();
				if(GameState == STATE_TRANSITION)
					Graphics::RenderTransition();
			}
			if(FadingState != FADING_STATE_NONE)
				Graphics::DrawFading();
			if(GameState == STATE_PAUSED)
				Graphics::RenderMenuItems(MENU_PAUSE);
			//DrawFPS(dtG);
			Graphics::WindowUpdate();
			timeStart_graphics = clock::now();
		}

		// Workaround to allow for gapless ogg looping without bugs
		Sound::ProcessMusic();
		// prevent 100% usage of cpu
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}

	// I'm not sure if this is even necessary since the program is about to quit and lose all memory anyway
	// I mostly used it to get VLD to stop yelling at me :D
	Cleanup();
	//VLDReportLeaks();

	return 0;
}

bool CheckDebugConfig()
{
	std::ifstream infile("debugme");
	return infile.good();
}

void Cleanup()
{
	// let each file handle their own disposing (avoids giant bulky function)
	try
	{
		Graphics::Cleanup();
		LevelCleanup();
		EntityCleanup();
		InputCleanup();
		BindsCleanup();
		TilesCleanup();
		InterfaceCleanup();
		MenusCleanup();
		Sound::Cleanup();
	}
	catch(...) {};
	// close SDL
	SDL_Quit();
}