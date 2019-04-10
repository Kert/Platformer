#ifndef _config_h_
#define _config_h_

#include <string>

enum KEYBINDS
{
	BIND_UP,
	BIND_DOWN,
	BIND_LEFT,
	BIND_RIGHT,
	BIND_JUMP,
	BIND_FIRE,
	BIND_SWITCH,
	BIND_OK,
	BIND_BACK,
	BIND_ARROWUP,
	BIND_ARROWDOWN,
	BIND_ARROWL,
	BIND_ARROWR,
	BIND_ESCAPE,
	BIND_ENTER
};


// Number of configurable bindings. The others are hardcoreded and are not saved/loaded from file
// Put these bindings at the end of the enum
#define NUM_CONFIGURABLE_BINDS 9

void InitConfig();
void LoadDefaultBindings();
void LoadConfig();
void SaveConfig();
void BindingsCleanup();
void SetBinding(int code, int bind);
int GetBindingFromCode(int code);
const char *GetBindingName(int bind);
const char *GetFullscreenMode(int code);
std::string GetDeviceBindName(int code);
int GetBindingCode(int bind);

#endif
