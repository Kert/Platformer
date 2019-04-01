#include "menu.h"
#include <sstream>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "sound.h"
#include "transition.h"

// TODO: go cross-platform
#include <Windows.h>

int SelectedItem;
int BindingKey;
MENUS CurrentMenu;
std::map<MENUS, Menu*> menus;

extern Level *level;
extern int playerLives;
extern int fullscreenMode;
extern int volumeMusic;
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
int CreateMapSelectMenu();

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
						SetCurrentMenu(MENU_MAPSELECT);						
					}
					if(SelectedItem == 1)
					{
						SetCurrentMenu(MENU_OPTIONS);
					}
					if(SelectedItem == 2)
						GameEndFlag = true;
				}
				else if(CurrentMenu == MENU_MAPSELECT)
				{
					std::string currentLevel = menus.at(MENU_MAPSELECT)->GetItemInfo(SelectedItem)->text;
					SetCurrentTransition(TRANSITION_LEVELSTART);
					ChangeGamestate(STATE_TRANSITION);
					// force the loading screen to draw for one frame before we start loading
					RenderTransition();
					WindowUpdate();
					level = new Level(currentLevel);
				}
				else if(CurrentMenu == MENU_OPTIONS)
				{
					if(SelectedItem == 1)
						SetCurrentMenu(MENU_VIDEO_OPTIONS);
					else if(SelectedItem == 3)
						SetCurrentMenu(MENU_BINDS);
					else if(SelectedItem == 4)
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
				if(SelectedItem == 2)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume--;
					if(currentVolume < 0)
						currentVolume = 128;
					menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetMusicVolume(currentVolume);
				}					
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
				if(SelectedItem == 2)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume++;
					if(currentVolume > 128)
						currentVolume = 0;
					menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetMusicVolume(currentVolume);
				}					
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

MenuItem::MenuItem(int x, int y, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor, TEXT_ALIGN align)
{
	pos.x = x;
	pos.y = y;
	this->text = text;
	this->font = font;
	this->standardColor = standardColor;
	this->selectedColor = selectedColor;
	this->align = align;
}

MenuItem::MenuItem(SDL_Point pos, std::string text, TTF_Font *font, SDL_Color standardColor, SDL_Color selectedColor)
{
	this->pos = pos;
	this->text = text;
	this->font = font;
	this->standardColor = standardColor;
	this->selectedColor = selectedColor;
	this->align = TEXT_ALIGN_CENTER;
}

void MenuItem::SetText(std::string text)
{
	this->text = text;
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
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4), "Start game", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 100, "Options", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 100 * 2, "Exit", menu_font, menu_color, selected_color));
	menus[MENU_MAIN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) - 100, "Lives: ", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4), "Video Options", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 100, "Music Volume", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 200, "Keybinds", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 300, "Back", menu_font, menu_color, selected_color));
	menus[MENU_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.4), GetWindowNormalizedY(0.4) - 100, "Display: ", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.4), GetWindowNormalizedY(0.4), "Mode: ", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.4), GetWindowNormalizedY(0.4) + 100, "Fullscreen:", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.4), GetWindowNormalizedY(0.4) + 100 * 2, "Back", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menus[MENU_VIDEO_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 100, "Resume", game_font, pause_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "Quit", game_font, pause_color, selected_color));
	menus[MENU_PAUSE] = menu;

	menu = new Menu();
	for(int i = 0; i < MAX_LIVES; i++)
	{
		menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.6) + 30 * i, 150, std::to_string(i + 1), menu_font, menu_color, selected_color));
	}
	menu->IsHorizontal = true;
	menus[MENU_SELECTION_LIVES] = menu;

	menu = new Menu();
	menu->IsHorizontal = true;
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.6) + 30, GetWindowNormalizedY(0.4) + 100, std::to_string(volumeMusic), menu_font, menu_color, selected_color));
	menus[MENU_SELECTION_MUSIC_VOLUME] = menu;

	menu = new Menu();
	menu->IsHorizontal = true;
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY] = menu;

	menu = new Menu();
	menu->IsHorizontal = true;
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY_MODE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 100, "Off", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 120, GetWindowNormalizedY(0.4) + 100, "On", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 120 * 2, GetWindowNormalizedY(0.4) + 100, "Borderless", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->IsHorizontal = true;
	menus[MENU_SELECTION_FULLSCREEN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 50, "Retry Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus[MENU_PLAYER_FAILED] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus[MENU_PLAYER_FAILED_NO_ESCAPE] = menu;

	menu = new Menu();
	menus[MENU_MAPSELECT] = menu;
	CreateMapSelectMenu();
	// insert bind and bindings menu here

	CreateDisplayMenu();
	RefreshDisplayModeMenus();
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
	for(auto &menu : menus)
	{
		delete menu.second;
	}
	std::map<MENUS, Menu*>().swap(menus); // forcibly deallocate memory
}

int CreateDisplayMenu()
{
	for(auto display : displayModes)
	{
		std::ostringstream modeName;
		// TODO: Proper UTF-8 text rendering
		//modeName << SDL_GetDisplayName(display.first);
		modeName << display.first;
		menus.at(MENU_SELECTION_DISPLAY)->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) - 100, modeName.str(), menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	}
	return 0;
}

std::vector<std::string> GetAllTiledMaps()
{
	std::vector<std::string> names;
	std::string search_path = "assets/levels/*.tmx";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while(::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

int CreateMapSelectMenu()
{
	std::vector<std::string> maps = GetAllTiledMaps();
	for(int i = 0; i < (int)maps.size(); i++)
	{
		menus.at(MENU_MAPSELECT)->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.2), GetWindowNormalizedY(0.2) + i * 50, maps[i], menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
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
		menus.at(MENU_SELECTION_DISPLAY_MODE)->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4), modeName.str(), menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
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