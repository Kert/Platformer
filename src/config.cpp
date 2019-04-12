#include "config.h"
#include <SDL.h>
#include <algorithm>
#include <map>
#include <fstream>
#include <string>
#include "INIReader.h"
#include "globals.h"
#include "sound.h"
#include "utils.h"

std::map<KEYBINDS, SDL_Keycode> bindsKeyboard;
std::map<KEYBINDS, Uint8> bindsController;

extern int fullscreenMode;
extern SDL_DisplayMode displayMode;
extern int displayIndex;

const std::map<KEYBINDS, std::string> bindNames = {
	{ BIND_UP,        "UP"},
	{ BIND_DOWN,      "DOWN" },
	{ BIND_LEFT,      "LEFT"},
	{ BIND_RIGHT,     "RIGHT"},
	{ BIND_JUMP,      "JUMP" },
	{ BIND_FIRE,      "FIRE" },
	{ BIND_SWITCH,    "SWITCH" },
	{ BIND_OK,        "OK" },
	{ BIND_BACK,      "BACK" },
	{ BIND_ARROWUP,   "ARROWUP" },
	{ BIND_ARROWDOWN, "ARROWDOWN" },
	{ BIND_ARROWL,    "ARROWL" },
	{ BIND_ARROWR,    "ARROWR" },
	{ BIND_ESCAPE,    "ESCAPE" },
	{ BIND_ENTER,     "BIND_ENTER" }
};

const std::vector<KEYBINDS> bindableKeys = {
	BIND_UP,
	BIND_DOWN,
	BIND_LEFT,
	BIND_RIGHT,
	BIND_JUMP,
	BIND_FIRE,
	BIND_SWITCH,
	BIND_OK,
	BIND_BACK
};

int GetNumConfigurableBinds()
{
	return bindableKeys.size();
}

std::vector<KEYBINDS> GetBindables()
{
	return bindableKeys;
}

std::map<std::string, int> fullscreenModes = {
	{"Off", 0},
	{"On", 1},
	{"Borderless", 2}
};

void InitConfig()
{
	LoadDefaultBinds();
	LoadConfig();
}

void LoadDefaultBinds()
{
	SetKeyboardBind(SDLK_UP, BIND_UP);
	SetKeyboardBind(SDLK_DOWN, BIND_DOWN);
	SetKeyboardBind(SDLK_LEFT, BIND_LEFT);
	SetKeyboardBind(SDLK_RIGHT, BIND_RIGHT);
	SetKeyboardBind(SDLK_z, BIND_JUMP);
	SetKeyboardBind(SDLK_x, BIND_FIRE);
	SetKeyboardBind(SDLK_c, BIND_SWITCH);
	SetKeyboardBind(SDLK_RETURN, BIND_OK);
	SetKeyboardBind(SDLK_ESCAPE, BIND_BACK);
	SetKeyboardBind(SDLK_UP, BIND_ARROWUP);
	SetKeyboardBind(SDLK_DOWN, BIND_ARROWDOWN);
	SetKeyboardBind(SDLK_LEFT, BIND_ARROWL);
	SetKeyboardBind(SDLK_RIGHT, BIND_ARROWR);
	SetKeyboardBind(SDLK_ESCAPE, BIND_ESCAPE);
	SetKeyboardBind(SDLK_RETURN2, BIND_ENTER);
	
	SetControllerBind(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, BIND_RIGHT);
	SetControllerBind(SDL_CONTROLLER_BUTTON_DPAD_LEFT, BIND_LEFT);
	SetControllerBind(SDL_CONTROLLER_BUTTON_DPAD_UP, BIND_UP);
	SetControllerBind(SDL_CONTROLLER_BUTTON_DPAD_DOWN, BIND_DOWN);
	SetControllerBind(SDL_CONTROLLER_BUTTON_A, BIND_JUMP);
	SetControllerBind(SDL_CONTROLLER_BUTTON_X, BIND_FIRE);
	SetControllerBind(SDL_CONTROLLER_BUTTON_B, BIND_SWITCH);
	SetControllerBind(SDL_CONTROLLER_BUTTON_A, BIND_OK);
	SetControllerBind(SDL_CONTROLLER_BUTTON_BACK, BIND_BACK);
	SetControllerBind(SDL_CONTROLLER_BUTTON_START, BIND_ESCAPE);
}

void LoadConfig()
{
	INIReader reader("config.ini");

	if(reader.ParseError() < 0)
	{
		PrintLog(LOG_IMPORTANT, "Can't load config.ini");
		return;
	}

	for(auto &i : bindNames)
	{
		std::string keyName = reader.Get("KeysKeyboard", i.second, "");
		if(keyName == "")
			PrintLog(LOG_IMPORTANT, "Wrong bind");
		else
			SetKeyboardBind(atoi(keyName.c_str()), i.first);
		keyName = reader.Get("KeysController", i.second, "");
		if(keyName == "")
			PrintLog(LOG_IMPORTANT, "Wrong bind");
		else
			SetControllerBind(atoi(keyName.c_str()), i.first);
	}

	fullscreenMode = fullscreenModes[reader.Get("Video", "Fullscreen", "Off")];
	displayIndex = atoi(reader.Get("Video", "Display", "0").c_str());
	displayMode.w = atoi(reader.Get("Video", "Width", "640").c_str());
	displayMode.h = atoi(reader.Get("Video", "Height", "480").c_str());
	displayMode.refresh_rate = atoi(reader.Get("Video", "RefreshRate", "60").c_str());
	displayMode.format = std::stoul(reader.Get("Video", "Format", "0").c_str());
	SetMusicVolume(atoi(reader.Get("Sound", "Music", "100").c_str()));
	SetSfxVolume(atoi(reader.Get("Sound", "Sfx", "100").c_str()));
}

void SaveConfig()
{
	// This overwrites all previous content in the file with current bindings
	// OK for now but if we want to add non-binding stuff later it becomes much more complicated to do
	std::ofstream file("config.ini");
	if(!file.good())
	{
		PrintLog(LOG_IMPORTANT, "File saving failed wtf");
		return;
	}
	file << "[KeysKeyboard]" << std::endl;
	for(auto i : bindsKeyboard)
	{
		file << GetBindingName(i.first) << "=" << i.second << std::endl;
	}
	file << "[KeysController]" << std::endl;
	for(auto i : bindsController)
	{
		file << GetBindingName(i.first) << "=" << (int)i.second << std::endl;
	}

	file << "[Video]" << std::endl;
	file << "Fullscreen=" << GetFullscreenMode(fullscreenMode) << std::endl;
	file << "Display=" << displayIndex << std::endl;
	file << "Width=" << displayMode.w << std::endl;
	file << "Height=" << displayMode.h << std::endl;
	file << "RefreshRate=" << displayMode.refresh_rate << std::endl;
	file << "Format=" << displayMode.format << std::endl;

	file << "[Sound]" << std::endl;
	file << "Music=" << GetMusicVolume() << std::endl;
	file << "Sfx=" << GetSfxVolume() << std::endl;
}

void SetKeyboardBind(SDL_Keycode code, KEYBINDS bind)
{
	bindsKeyboard[bind] = code;
}

void SetControllerBind(Uint8 code, KEYBINDS bind)
{
	bindsController[bind] = code;
}

int GetKeyboardBindFromCode(SDL_Keycode code)
{
	for(auto i : bindsKeyboard)
	{
		if (i.second == code)
			return i.first;
	}
	return -1;
}

SDL_Keycode GetKeyboardCodeFromBind(KEYBINDS bind)
{
	if (bindsKeyboard.find(bind) != bindsKeyboard.end())
		return bindsKeyboard[bind];
	return -1;
}

int GetControllerBindFromCode(Uint8 code)
{
	for(auto i : bindsController)
	{
		if (i.second == code)
			return i.first;
	}
	return -1;
}

Uint8 GetControllerCodeFromBind(KEYBINDS bind)
{
	if (bindsController.find(bind) != bindsController.end())
		return bindsController[bind];
	return -1;
}

std::string GetBindingName(KEYBINDS bind)
{
	if (bindNames.find(bind) != bindNames.end())
		return bindNames.at(bind);
	else
		return "---";
}

std::string GetFullscreenMode(int code)
{
	switch(code)
	{
	case 0:
		return "Off";
	case 1:
		return "On";
	case 2:
		return "Borderless";
	default:
		return "";
	}
}

std::string GetKeyboardKeyName(SDL_Keycode code)
{
	const char* name = SDL_GetKeyName(code);
	if (name)
	{
		std::string str = name;
		std::transform(str.begin(), str.end(),str.begin(), ::toupper);
		return str;
	}
	else
		return "---";
}

std::string GetControllerKeyName(Uint8 code)
{
	const char* name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)code);
	if (name)
	{
		std::string str = name;
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}
	else
		return "---";
}
	
void BindsCleanup()
{
	std::map<KEYBINDS, SDL_Keycode>().swap(bindsKeyboard); // forcibly deallocate memory
	std::map<KEYBINDS, Uint8>().swap(bindsController); // forcibly deallocate memory
}
