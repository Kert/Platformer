#ifndef _config_h_
#define _config_h_

#include <SDL.h>
#include <set>
#include <string>
#include "globals.h"

int GetNumConfigurableBinds();
std::vector<KEYBINDS> GetBindables();

void InitConfig();
void LoadDefaultBinds();
void LoadConfig();
void SaveConfig();
void BindsCleanup();
void SetKeyboardBind(SDL_Keycode code, KEYBINDS bind);
void SetControllerBind(Uint8 code, KEYBINDS bind);
int GetKeyboardBindFromCode(SDL_Keycode code);
int GetControllerBindFromCode(Uint8 code);
SDL_Keycode GetKeyboardCodeFromBind(KEYBINDS bind);
Uint8 GetControllerCodeFromBind(KEYBINDS bind);

std::string GetBindingName(KEYBINDS bind);
std::string GetFullscreenModeName(int code);
std::string GetScalingModeName(int mode);
std::string GetKeyboardKeyName(SDL_Keycode code);
std::string GetControllerKeyName(Uint8 code);

#endif
