#include "interface.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <map>
#include <sstream>
#include "graphics.h"
#include "utils.h"

// interface structure, in order:
// int - which part of the interface this is (affects draw priority)
// SDL_Rect* - the part of the texture being used (allows different drawn frames)
// SDL_Rect* - where on the screen the interface part will draw
// SDL_Texture* - the actual texture being drawn
std::map<int, InterfacePiece> interfaces;

std::map<int, InterfacePiece>* GetInterfaces()
{
	return &interfaces;
}

void InterfaceSetup()
{	
	BuildInterface(32, 32, 20, 20, "assets/sprites/lifebar.png", 0, INTERFACE_LIFE);
	BuildInterface(32, 32, 20, 20, "assets/sprites/abilities.png", 0, INTERFACE_ABILITY);
	//BuildInterface(TEXT_HEIGHT, SCORE_WIDTH, SCORE_OFFSET_X, SCORE_OFFSET_Y, "000000", -1, INTERFACE_SCORE);
	BuildInterface(32, 32 * 5, 52, 20, "00:00", -1, INTERFACE_TIME);
}

std::string InfoFormat(int data1, int data2)
{
	std::stringstream o;
	o << AddLeadingZeroes(data1, 2) << ":" << AddLeadingZeroes(data2, 2);
	return o.str();
}

std::string InfoFormat(std::string data1, int data2)
{
	std::stringstream o;
	o << data1 << ":" << AddLeadingZeroes(data2, 2);
	return o.str();
}

void PrintNumToInterface(int num, int part, int length)
{
	interfaces.at(part).text = AddLeadingZeroes(num, length);
}

// todo: condense with num method?
void PrintToInterface(std::string data, int part)
{
	interfaces.at(part).text = data;
}

void ChangeInterfaceFrame(int frame, int part)
{
	if(part >= (int)interfaces.size())
	{
		PrintLog(LOG_DEBUG, "Attempted to change interface frame for unexisting interface");
		return;
	}
	interfaces.at(part).frame.x = interfaces.at(part).frame.w * frame;
}

void BuildInterface(int h, int w, int x, int y, const char* content, int frame, int part)
{
	SDL_Rect f;
	f.h = h;
	f.w = w;
	f.x = (w * frame);
	f.y = 0;

	SDL_Rect r;
	r.h = h;
	r.w = w;
	r.x = x;
	r.y = y;

	if(frame == -1) // null frame, treat as text
	{
		interfaces[part].text = content;
		interfaces[part].tex = NULL;
	}
	else
		interfaces[part].tex = IMG_LoadTexture(Graphics::GetRenderer(), content);

	interfaces[part].frame = f;
	interfaces[part].location = r;
}

void InterfaceCleanup()
{
	for(auto i : interfaces)
		SDL_DestroyTexture(i.second.tex);
	std::map<int, InterfacePiece>().swap(interfaces); // forcibly deallocate memory
}