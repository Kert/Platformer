#include "menu.h"
#include <sstream>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "sound.h"
#include "transition.h"

// TODO: go cross-platform
#include <Windows.h>

KEYBINDS BindingKey;
MENUS CurrentMenu;
std::map<MENUS, Menu*> menus;

extern Level *level;
extern int currentLives;
extern int playerLives;
extern int fullscreenMode;
extern bool GameEndFlag;
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

void DoMenuAction(int kbkey, int jbutton, int bind)
{
	if(CurrentMenu == MENU_BINDKEY) // special case for keybind screen
	{
		if(bind != BIND_ESCAPE)
		{
			if(jbutton != 255)
				SetControllerBind(jbutton, (KEYBINDS)BindingKey);
			else
				SetKeyboardBind(kbkey, (KEYBINDS)BindingKey);
		}
			
		SetCurrentMenu(MENU_BINDS);
	}
	else
	{
		int SelectedItem = menus.at(CurrentMenu)->selected;
		switch(bind)
		{
			case BIND_UP: case BIND_DOWN: case BIND_LEFT: case BIND_RIGHT:
			case BIND_ARROWUP: case BIND_ARROWDOWN: case BIND_ARROWL: case BIND_ARROWR:
				NavigateMenu(bind);
				break;
			case BIND_JUMP: case BIND_OK: case BIND_ENTER:
				PlaySfx("menu-confirm");
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
					currentLives = playerLives;
				}
				else if(CurrentMenu == MENU_OPTIONS)
				{
					if(SelectedItem == 0)
						SetCurrentMenu(MENU_VIDEO_OPTIONS);
					else if(SelectedItem == 1)
						SetCurrentMenu(MENU_SOUND_OPTIONS);
					else if(SelectedItem == 2)
						SetCurrentMenu(MENU_BINDS);
					else if(SelectedItem == 3)
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
				else if(CurrentMenu == MENU_SOUND_OPTIONS)
				{
					if(SelectedItem == 2)
						SetCurrentMenu(MENU_OPTIONS);
				}
				else if(CurrentMenu == MENU_BINDS)
				{
					if(SelectedItem < (menus.at(CurrentMenu)->GetItemCount() - 2))
					{
						BindingKey = static_cast<KEYBINDS>(SelectedItem); // the enums match up, so we can do this mini-optimization :D
						SetCurrentMenu(MENU_BINDKEY);
					}
					else
					{
						if(SelectedItem == (menus.at(CurrentMenu)->GetItemCount() - 2))
							LoadDefaultBinds();
						SetCurrentMenu(MENU_OPTIONS);
					}
				}
				else if(CurrentMenu == MENU_PAUSE)
				{
					if(SelectedItem == 0)
					{
						ChangeGamestate(STATE_GAME);
						ResumeMusic();
					}
					else if(SelectedItem == 1)
					{
						delete level;
						level = nullptr;
						ChangeGamestate(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
						StopMusic();
					}
				}
				else if(CurrentMenu == MENU_PLAYER_FAILED)
				{
					if(SelectedItem == 0)
					{
						level->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						ChangeGamestate(STATE_TRANSITION);
					}
					else if(SelectedItem == 1)
					{
						level->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						ChangeGamestate(STATE_TRANSITION);
						currentLives = playerLives;
					}
					else if(SelectedItem == 2)
					{
						delete level;
						level = nullptr;
						ChangeGamestate(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
					}
				}
				else if(CurrentMenu == MENU_PLAYER_FAILED_NO_ESCAPE)
				{
					if(SelectedItem == 0)
					{
						level->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						ChangeGamestate(STATE_TRANSITION);
						currentLives = playerLives;						
					}
					else if(SelectedItem == 1)
					{
						delete level;
						level = nullptr;
						ChangeGamestate(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
					}
				}
				break;
		}
	}
}

void NavigateMenu(int bind)
{
	int SelectedItem = menus.at(CurrentMenu)->selected;
	switch(bind)
	{
		case BIND_LEFT: case BIND_ARROWL:
			if(CurrentMenu == MENU_SOUND_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume--;
					if(currentVolume < 0)
						currentVolume = 128;
					menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetMusicVolume(currentVolume);
				}
				if(SelectedItem == 1)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume--;
					if(currentVolume < 0)
						currentVolume = 128;
					menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetSfxVolume(currentVolume);
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
			if(CurrentMenu == MENU_SOUND_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume++;
					if(currentVolume > 128)
						currentVolume = 0;
					menus.at(MENU_SELECTION_MUSIC_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetMusicVolume(currentVolume);
				}
				if(SelectedItem == 1)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume++;
					if(currentVolume > 128)
						currentVolume = 0;
					menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					SetSfxVolume(currentVolume);
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
	menus.at(CurrentMenu)->selected = SelectedItem;
	PlaySfx("menu-select");
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
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 5, GetWindowNormalizedY(0.5) - 32 * 3, "START GAME", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 4, GetWindowNormalizedY(0.5), "SETTINGS", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 2, GetWindowNormalizedY(0.5) + 32 * 3, "EXIT", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_MAIN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 6, "VIDEO", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 3, "SOUND", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "CONTROLS", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 3, "BACK", menu_font, menu_color, selected_color));
	menus[MENU_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 6, "DISPLAY:", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) - 32 * 5, GetWindowNormalizedY(0.5) - 32 * 3, "MODE:", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "FULLSCREEN:", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 3, "BACK", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_VIDEO_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 6, "MUSIC:", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) - 32 * 2, GetWindowNormalizedY(0.5) - 32 * 3, "SFX:", menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "BACK", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_SOUND_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 60, "RESUME", game_font, pause_color, selected_color, TEXT_ALIGN_CENTER));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 60, "QUIT", game_font, pause_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_PAUSE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 6, GetWindowNormalizedY(0.5) - 32 * 6, std::to_string(GetMusicVolume()), menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_SELECTION_MUSIC_VOLUME] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 6, GetWindowNormalizedY(0.5) - 32 * 3, std::to_string(GetSfxVolume()), menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_SELECTION_SFX_VOLUME] = menu;

	menu = new Menu();
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY] = menu;

	menu = new Menu();
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY_MODE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32, GetWindowNormalizedY(0.5), "OFF", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 5, GetWindowNormalizedY(0.5), "ON", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32 * 8, GetWindowNormalizedY(0.5), "BORDERLESS", menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menus[MENU_SELECTION_FULLSCREEN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "RETRY LEVEL", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 3, "NEW LEVEL", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 6, "BACK TO MENU", menu_font, menu_color, selected_color));
	menus[MENU_PLAYER_FAILED] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 3, "NEW LEVEL", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 6, "BACK TO MENU", menu_font, menu_color, selected_color));
	menus[MENU_PLAYER_FAILED_NO_ESCAPE] = menu;

	menu = new Menu();
	menus[MENU_MAPSELECT] = menu;
	CreateMapSelectMenu();
	
	menu = new Menu();
	
	int half = (GetNumConfigurableBinds() + 2) / 2;
	int i = 0;
	std::vector<KEYBINDS> bindables = GetBindables();
	for(auto bind : bindables)
	{
		int x = GetWindowNormalizedX(0.3) - 32;
		int y = GetWindowNormalizedY(0.5) - 32 * (half - i) * 2;
		menu->AddMenuItem(new MenuItem(x, y, GetBindingName(bind), menu_font, menu_color, selected_color, TEXT_ALIGN_RIGHT));
		i++;
	}

	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * (half - GetNumConfigurableBinds()) * 2, "RESET TO DEFAULT", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * (half - GetNumConfigurableBinds() - 1) * 2, "BACK", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_BINDS] = menu;

	menu = new Menu();
	menus[MENU_BINDKEY] = menu;

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
	int SelectedItem = 0;

	if(menu == MENU_VIDEO_OPTIONS)
	{
		RefreshDisplayModeMenus();
	}

	switch(oldmenu)
	{
		case MENU_OPTIONS:
			SaveConfig();
			if(menu == MENU_MAIN)
				SelectedItem = 1;
			break;
		case MENU_VIDEO_OPTIONS:
			SelectedItem = 0;
			break;
		case MENU_SOUND_OPTIONS:
			SelectedItem = 1;
			break;
		case MENU_BINDS:
			SelectedItem = 2;
			break;
		case MENU_BINDKEY:
			SelectedItem = BindingKey;
			break;
		default:
			SelectedItem = 0;
	}
	menus.at(CurrentMenu)->selected = SelectedItem;
}

MENUS GetCurrentMenu()
{
	return CurrentMenu;
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
		menus.at(MENU_SELECTION_DISPLAY)->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) + 32, GetWindowNormalizedY(0.5) - 32 * 6, modeName.str(), menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
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
	menu->IsSwitchable = true;
	menus.at(MENU_SELECTION_DISPLAY_MODE) = menu;

	for(auto mode : displayModes[displayIndex])
	{
		std::ostringstream modeName;
		modeName << mode.w << "x" << mode.h << "@" << mode.refresh_rate << "HZ " << SDL_BITSPERPIXEL(mode.format) << "-BIT";
		menus.at(MENU_SELECTION_DISPLAY_MODE)->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) - 32 * 4, GetWindowNormalizedY(0.5) - 32 * 3, modeName.str(), menu_font, menu_color, selected_color, TEXT_ALIGN_LEFT));
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