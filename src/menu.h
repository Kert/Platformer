#ifndef _menu_h_
#define _menu_h_

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include "config.h"
#include "globals.h"

// Uncommenting this enables borderless window option and support
// It needs proper scaling values to work right so it's disabled for now

//#define ALLOW_BORDERLESS

#ifdef ALLOW_BORDERLESS
	#define FULLSCREEN_MODES 2
#else
	#define FULLSCREEN_MODES 1
#endif

void DoMenuAction(int code, int bind);
void NavigateMenu(int bind);
void LoadMenus();
void SetCurrentMenu(MENUS menu);
void MenusCleanup();

class MenuItem;

class Menu
{
	private:
		std::vector<MenuItem*> items;
	public:
		bool IsHorizontal = false;
		int selected = 0;
	public:
		Menu();
		~Menu();
		void AddMenuItem(MenuItem *item);
		int GetItemCount();
		MenuItem* GetItemInfo(int number);
};

class MenuItem
{
	public:
		SDL_Point pos;
		std::string text;
		TTF_Font *font;
		SDL_Color standardColor;
		SDL_Color selectedColor;
		
	public:
		MenuItem(int x, int y, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor);
		MenuItem(SDL_Point pos, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor);
		~MenuItem();
};

#endif
