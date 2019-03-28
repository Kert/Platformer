#ifndef _menu_h_
#define _menu_h_

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include "config.h"
#include "globals.h"

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
		bool IsSwitchable = false;
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
		TEXT_ALIGN align;

	public:
		MenuItem(int x, int y, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor, TEXT_ALIGN align = TEXT_ALIGN_CENTER);
		MenuItem(SDL_Point pos, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor);
		void SetText(std::string text);
		~MenuItem();
};

#endif
