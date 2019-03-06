#include "interface.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <map>
#include <sstream>
#include "graphics.h"
#include "utils.h"

extern SDL_Renderer *renderer;

// interface structure, in order:
// int - which part of the interface this is (affects draw priority)
// SDL_Rect* - the part of the texture being used (allows different drawn frames)
// SDL_Rect* - where on the screen the interface part will draw
// SDL_Texture* - the actual texture being drawn
std::map<int, InterfacePiece> interface;

TTF_Font *font = NULL;

extern int RENDER_SCALE;

void InterfaceSetup()
{
	font = TTF_OpenFont("assets/misc/verdana.ttf", 14);
	BuildInterface(32, 32, 20, 20, "assets/sprites/lifebar.png", 0, INTERFACE_LIFE);
	BuildInterface(32, 32, 20, 20, "assets/sprites/abilities.png", 0, INTERFACE_ABILITY);
	//BuildInterface(TEXT_HEIGHT, SCORE_WIDTH, SCORE_OFFSET_X, SCORE_OFFSET_Y, "000000", -1, INTERFACE_SCORE);
	//BuildInterface(TEXT_HEIGHT, TIME_WIDTH, TIME_OFFSET_X, TIME_OFFSET_Y, "00:00", -1, INTERFACE_TIME);
}

const char* InfoFormat(int data1, int data2)
{
	std::stringstream o;
	const char* c1 = AddLeadingZeroes(data1, 2);
	const char* c2 = AddLeadingZeroes(data2, 2);
	o << c1;
	delete c1;
	o << ":";
	o << c2;
	delete c2;
	std::string temp = o.str().c_str();

	return StringToChars(temp);
}

const char* InfoFormat(const char* data1, int data2)
{
	std::stringstream o;
	const char* c = AddLeadingZeroes(data2, 2);
	o << data1;
	o << ":";
	o << c;
	delete c;
	//delete data1;
	std::string temp = o.str().c_str();

	return StringToChars(temp);
}

void PrintNumToInterface(int num, int part, int length)
{
	const char* toPrint = AddLeadingZeroes(num, length);
	interface.at(part).text = toPrint;
	delete toPrint;
}

// todo: condense with num method?
void PrintToInterface(const char* data, int part)
{
	interface.at(part).text = data;
	delete data;
}

void ChangeInterfaceFrame(int frame, int part)
{
	if(part >= (int)interface.size())
	{
		PrintLog(LOG_DEBUG, "Attempted to change interface frame for unexisting interface");
		return;
	}
	interface.at(part).frame.x = interface.at(part).frame.w * frame;
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
		interface[part].text = content;
		interface[part].tex = NULL;
	}
	else
		interface[part].tex = IMG_LoadTexture(renderer, content);

	interface[part].frame = f;
	interface[part].location = r;
}

void RenderInterface()
{
	SDL_Color interface_color = { 50, 180, 0 };
	for(auto iter : interface)
	{
		if(iter.second.tex == NULL)
			RenderText(iter.second.location.x, iter.second.location.y, iter.second.text, font, interface_color);
		else
		{
			SDL_Rect dest;
			dest.x = iter.second.location.x * RENDER_SCALE;
			dest.y = iter.second.location.y * RENDER_SCALE;
			dest.w = iter.second.location.w * RENDER_SCALE;
			dest.h = iter.second.location.h * RENDER_SCALE;
			SDL_RenderCopy(renderer, iter.second.tex, &iter.second.frame, &dest);
		}
	}
}

void InterfaceCleanup()
{
	for(auto i : interface)
		SDL_DestroyTexture(i.second.tex);
	std::map<int, InterfacePiece>().swap(interface); // forcibly deallocate memory
}
