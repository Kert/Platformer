#include "main.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <chrono>
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
#include "utils.h"


void Cleanup();

int main(int argc, char* argv[])
{
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
	Game::SetState(STATE_TRANSITION);

	// Fixed time step game loop wizardry
	using namespace std::chrono_literals;
	using clock = std::chrono::high_resolution_clock;
	auto timeStart_graphics = clock::now();
	// main loop
	while(!Game::IsGameEndRequested())
	{
		InputUpdate();

		double ticksMultiplier = 1. / (Graphics::GetRefreshRate() / 60.);

		if(Fading::GetState() != FADING_STATE_NONE)
			Fading::Update(ticksMultiplier);

		if(Game::GetState() == STATE_GAME && Fading::GetState() != FADING_STATE_BLACKNBACK)
		{
			Game::Update(ticksMultiplier);
		}

		auto deltaTime_graphics = clock::now() - timeStart_graphics;
		timeStart_graphics = clock::now();

		Graphics::WindowFlush();
		if(Game::GetState() == STATE_GAME || Game::GetState() == STATE_PAUSED)
		{
			Graphics::Update(ticksMultiplier);
		}
		else
		{
			if(Game::GetState() == STATE_MENU)
				Graphics::RenderMenu();
			if(Game::GetState() == STATE_TRANSITION)
				Graphics::RenderTransition();
		}
		if(Fading::GetState() != FADING_STATE_NONE)
			Graphics::DrawFading();
		if(Game::GetState() == STATE_PAUSED)
			Graphics::RenderMenuItems(MENU_PAUSE);
		if(Game::IsDebug())
			Graphics::DrawFPS(deltaTime_graphics.count());
		Graphics::WindowUpdate();
		// Workaround to allow for gapless ogg looping without bugs
		Sound::ProcessMusic();
		// prevent 100% usage of cpu
		// no longer needed because of vsync
		//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}

	// I'm not sure if this is even necessary since the program is about to quit and lose all memory anyway
	// I mostly used it to get VLD to stop yelling at me :D
	Cleanup();
	//VLDReportLeaks();

	return 0;
}

void Cleanup()
{
	// let each file handle their own disposing (avoids giant bulky function)
	try
	{
		Graphics::Cleanup();
		Game::RemoveLevel();
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