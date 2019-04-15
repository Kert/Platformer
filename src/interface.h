#ifndef _interface_h_
#define _interface_h_ 

#include <SDL.h>
#include <map>
#include <string>

struct InterfacePiece
{
	SDL_Rect frame;
	SDL_Rect location;
	SDL_Texture *tex;
	std::string text;
};

void InterfaceSetup();
const char* InfoFormat(const char* data1, int data2);
const char* InfoFormat(int data1, int data2);
void PrintNumToInterface(int num, int part, int length);
void PrintToInterface(const char* data, int part);
void ChangeInterfaceFrame(int frame, int part);
void BuildInterface(int h, int w, int x, int y, const char* content, int frame, int part);
void InterfaceCleanup();
std::map<int, InterfacePiece>* GetInterfaces();

enum INTERFACE_PARTS
{
	INTERFACE_LIFE,
	INTERFACE_ABILITY,
	INTERFACE_SCORE,
	INTERFACE_TIME
};

#endif
