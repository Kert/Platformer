#include "menu.h"
#include <sstream>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "transition.h"

int SelectedItem;
int BindingKey;
MENUS CurrentMenu;
std::vector<Menu*> menus;

extern Level *level;
extern int playerLives;
extern int fullscreenMode;
extern bool GameEndFlag;
extern TTF_Font *minor_font;
extern TTF_Font *menu_font;
extern TTF_Font *game_font;
extern SDL_Color menu_color;
extern SDL_Color selected_color;
extern SDL_Color pause_color;

extern int displayIndex;
extern SDL_DisplayMode displayMode;
extern std::map<int, std::vector<SDL_DisplayMode>> displayModes;

int RefreshDisplayModeMenus();
int CreateDisplayMenu();

void DoMenuAction(int code, int bind)
{
	if(CurrentMenu == MENU_BIND) // special case for keybind screen
	{
		if(bind != BIND_ESCAPE)
			SetBinding(code, BindingKey);
		SetCurrentMenu(MENU_BINDS);
	}
	else
	{
		switch(bind)
		{
			case BIND_UP: case BIND_DOWN: case BIND_LEFT: case BIND_RIGHT:
			case BIND_ARROWUP: case BIND_ARROWDOWN: case BIND_ARROWL: case BIND_ARROWR:
				NavigateMenu(bind);
				break;
			case BIND_JUMP: case BIND_OK: case BIND_ENTER:
				if(CurrentMenu == MENU_MAIN)
				{
					if(SelectedItem == 0)
					{
						SetCurrentTransition(TRANSITION_LEVELSTART);
						ChangeGamestate(STATE_TRANSITION);
						// force the loading screen to draw for one frame before we start loading
						UpdateTransition();
						WindowUpdate();
						level = new Level();
					}
					if(SelectedItem == 1)
					{
						SetCurrentMenu(MENU_OPTIONS);
					}
					if(SelectedItem == 2)
						GameEndFlag = true;
				}
				else if(CurrentMenu == MENU_OPTIONS)
				{
					if(SelectedItem == 1)
						SetCurrentMenu(MENU_VIDEO_OPTIONS);
					if(SelectedItem == 2)
						SetCurrentMenu(MENU_BINDS);
					if(SelectedItem == 3)
						SetCurrentMenu(MENU_MAIN);
				}
				else if(CurrentMenu == MENU_VIDEO_OPTIONS)
				{
					if(SelectedItem == 0)
						UpdateDisplayMode();
					if(SelectedItem == 1)
					{
						displayMode = displayModes[displayIndex][menus.at(MENU_SELECTION_DISPLAY_MODE)->selected];
						UpdateDisplayMode();
					}
					if(SelectedItem == 3)
						SetCurrentMenu(MENU_OPTIONS);
				}
				else if(CurrentMenu == MENU_BINDS)
				{
					if(SelectedItem < (menus.at(CurrentMenu)->GetItemCount() - 2))
					{
						BindingKey = SelectedItem; // the enums match up, so we can do this mini-optimization :D
						SetCurrentMenu(MENU_BIND);
					}
					else
					{
						if(SelectedItem == (menus.at(CurrentMenu)->GetItemCount() - 2))
							LoadDefaultBindings();
						SetCurrentMenu(MENU_OPTIONS);
					}
				}
				break;
		}
	}
}

void NavigateMenu(int bind)
{
	switch(bind)
	{
		case BIND_LEFT: case BIND_ARROWL:
			if(CurrentMenu == MENU_OPTIONS)
			{
				if(SelectedItem == 0)
					menus.at(MENU_SELECTION_LIVES)->selected <= 0 ? menus.at(MENU_SELECTION_LIVES)->selected = (menus.at(MENU_SELECTION_LIVES)->GetItemCount() - 1) : menus.at(MENU_SELECTION_LIVES)->selected--;
			}
			if(CurrentMenu == MENU_VIDEO_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					menus.at(MENU_SELECTION_DISPLAY)->selected >= (menus.at(MENU_SELECTION_DISPLAY)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_DISPLAY)->selected = 0 : menus.at(MENU_SELECTION_DISPLAY)->selected++;
					displayIndex = menus.at(MENU_SELECTION_DISPLAY)->selected;
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 1)
					menus.at(MENU_SELECTION_DISPLAY_MODE)->selected >= (menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = 0 : menus.at(MENU_SELECTION_DISPLAY_MODE)->selected++;
				else if(SelectedItem == 2)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected <= 0 ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) : menus.at(MENU_SELECTION_FULLSCREEN)->selected--;
					fullscreenMode = menus.at(MENU_SELECTION_FULLSCREEN)->selected;
					UpdateDisplayMode();
					RefreshDisplayModeMenus();
				}
			}
			break;
		case BIND_RIGHT: case BIND_ARROWR:
			if(CurrentMenu == MENU_OPTIONS)
			{
				if(SelectedItem == 0)
					menus.at(MENU_SELECTION_LIVES)->selected >= (menus.at(MENU_SELECTION_LIVES)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_LIVES)->selected = 0 : menus.at(MENU_SELECTION_LIVES)->selected++;
			}
			if(CurrentMenu == MENU_VIDEO_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					menus.at(MENU_SELECTION_DISPLAY)->selected <= 0 ? menus.at(MENU_SELECTION_DISPLAY)->selected = (menus.at(MENU_SELECTION_DISPLAY)->GetItemCount() - 1) : menus.at(MENU_SELECTION_DISPLAY)->selected--;
					displayIndex = menus.at(MENU_SELECTION_DISPLAY)->selected;
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 1)
					menus.at(MENU_SELECTION_DISPLAY_MODE)->selected <= 0 ? menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = (menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1) : menus.at(MENU_SELECTION_DISPLAY_MODE)->selected--;
				else if(SelectedItem == 2)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected >= (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = 0 : menus.at(MENU_SELECTION_FULLSCREEN)->selected++;
					fullscreenMode = menus.at(MENU_SELECTION_FULLSCREEN)->selected;
					UpdateDisplayMode();
					RefreshDisplayModeMenus();
				}
			}
			break;
		case BIND_UP: case BIND_ARROWUP:
			SelectedItem <= 0 ? SelectedItem = (menus.at(CurrentMenu)->GetItemCount() - 1) : SelectedItem--;
			break;
		case BIND_DOWN: case BIND_ARROWDOWN:
			SelectedItem >= (menus.at(CurrentMenu)->GetItemCount() - 1) ? SelectedItem = 0 : SelectedItem++;
			break;
	}
}

MenuItem::~MenuItem()
{
}

MenuItem::MenuItem(int x, int y, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor)
{
	pos.x = x;
	pos.y = y;
	this->text = text;
	this->font = font;
	this->standardColor = standardColor;
	this->selectedColor = selectedColor;
}

MenuItem::MenuItem(SDL_Point pos, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor)
{
	this->pos = pos;
	this->text = text;
	this->font = font;
	this->standardColor = standardColor;
	this->selectedColor = selectedColor;
}

Menu::Menu()
{
}

Menu::~Menu()
{
	while(items.size())
	{
		delete items.back();
		items.pop_back();
	}
}

void Menu::AddMenuItem(MenuItem *item)
{
	items.push_back(item);
}

int Menu::GetItemCount()
{
	return items.size();
}

void LoadMenus()
{
	Menu *menu = new Menu();
	menu->AddMenuItem(new MenuItem(315, 220, "Start game", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(315, 300, "Options", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(315, 380, "Exit", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(70, 150, "Lives: ", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 230, "Video Options", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 390, "Keybinds", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 470, "Back", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(70, 150, "Display: ", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 230, "Mode: ", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 310, "Fullscreen:", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(70, 470, "Back", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() - 100, "Resume", game_font, pause_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY(), "Quit", game_font, pause_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	for(int i = 0; i < MAX_LIVES; i++)
	{
		menu->AddMenuItem(new MenuItem(300 + 30 * i, 150, std::to_string(i + 1), menu_font, menu_color, selected_color));
	}
	menu->IsHorizontal = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->IsHorizontal = true;
	menu->IsSwitchable = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->IsHorizontal = true;
	menu->IsSwitchable = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(300, 310, "Off", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(370, 310, "On", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(440, 310, "Borderless", menu_font, menu_color, selected_color));

	menu->IsHorizontal = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() + 50, "Retry Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetScreenCenterX(), GetScreenCenterY() + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus.push_back(menu);
	// insert bind and bindings menu here

	CreateDisplayMenu();
}

MenuItem* Menu::GetItemInfo(int number)
{
	return items.at(number);
}

void SetCurrentMenu(MENUS menu)
{
	MENUS oldmenu;
	oldmenu = CurrentMenu;
	CurrentMenu = menu;
	SelectedItem = 0;
	if(menu == MENU_OPTIONS)
	{
		menus.at(MENU_SELECTION_LIVES)->selected = playerLives - 1;
	}
	if(menu == MENU_VIDEO_OPTIONS)
	{
		RefreshDisplayModeMenus();		
	}
	if(oldmenu == MENU_OPTIONS)
	{
		playerLives = menus.at(MENU_SELECTION_LIVES)->selected + 1;
		SaveConfig();
		if(menu == MENU_MAIN)
			SelectedItem = 1;
	}
}

void MenusCleanup()
{
	while(menus.size())
	{
		delete menus.back();
		menus.pop_back();
	}
	std::vector<Menu*>().swap(menus); // forcibly deallocate memory
}

int CreateDisplayMenu()
{
	for(auto display : displayModes)
	{
		std::ostringstream modeName;
		// TODO: Proper UTF-8 text rendering
		//modeName << SDL_GetDisplayName(display.first);
		modeName << display.first;
		menus.at(MENU_SELECTION_DISPLAY)->AddMenuItem(new MenuItem(300, 160, modeName.str(), menu_font, menu_color, selected_color));
	}
	return 0;
}

int RefreshDisplayModeMenus()
{
	delete menus.at(MENU_SELECTION_DISPLAY_MODE);
	Menu *menu = new Menu();
	menu->IsHorizontal = true;
	menu->IsSwitchable = true;
	menus.at(MENU_SELECTION_DISPLAY_MODE) = menu;

	for(auto mode : displayModes[displayIndex])
	{
		std::ostringstream modeName;
		modeName << mode.w << "x" << mode.h << "@" << mode.refresh_rate << "hz " << SDL_BITSPERPIXEL(mode.format) << "-bit";
		menus.at(MENU_SELECTION_DISPLAY_MODE)->AddMenuItem(new MenuItem(300, 230, modeName.str(), menu_font, menu_color, selected_color));
		if(displayMode.w == mode.w &&
			displayMode.h == mode.h &&
			displayMode.refresh_rate == mode.refresh_rate &&
			displayMode.format == mode.format)
			menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1;
	}

	menus.at(MENU_SELECTION_FULLSCREEN)->selected = fullscreenMode;
	menus.at(MENU_SELECTION_DISPLAY)->selected = displayIndex;
	return 0;
}