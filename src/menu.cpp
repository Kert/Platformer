#include "menu.h"
#include <sstream>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "sound.h"
#include "transition.h"
#include "utils.h"

KEYBINDS BindingKey;
MENUS CurrentMenu;
std::map<MENUS, Menu*> menus;

extern TTF_Font *menu_font;
extern TTF_Font *game_font;
extern SDL_Color menu_color;
extern SDL_Color selected_color;
extern SDL_Color pause_color;

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
				Sound::PlaySfx("menu-confirm");
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
						Game::SetGameEndFlag();
				}
				else if(CurrentMenu == MENU_MAPSELECT)
				{
					std::string currentLevel = menus.at(MENU_MAPSELECT)->GetItemInfo(SelectedItem)->text;
					SetCurrentTransition(TRANSITION_LEVELSTART);
					Game::ChangeState(STATE_TRANSITION);
					// force the loading screen to draw for one frame before we start loading
					Graphics::RenderTransition();
					Graphics::WindowUpdate();
					Game::CreateLevel(currentLevel);
					Game::ResetPlayerLives();
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
						Graphics::UpdateDisplayMode();
					if(SelectedItem == 1)
					{
						Graphics::SetDisplayMode(Graphics::GetDisplayMode(Graphics::GetDisplayIndex(), menus.at(MENU_SELECTION_DISPLAY_MODE)->selected));
						Graphics::UpdateDisplayMode();
					}
					if(SelectedItem == 4)
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
						Game::ChangeState(STATE_GAME);
						Sound::ResumeMusic();
					}
					else if(SelectedItem == 1)
					{
						Game::RemoveLevel();
						Game::ChangeState(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
						Sound::StopMusic();
					}
				}
				else if(CurrentMenu == MENU_PLAYER_FAILED)
				{
					if(SelectedItem == 0)
					{
						Game::GetLevel()->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						Game::ChangeState(STATE_TRANSITION);
					}
					else if(SelectedItem == 1)
					{
						Game::GetLevel()->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						Game::ChangeState(STATE_TRANSITION);
						Game::ResetPlayerLives();
					}
					else if(SelectedItem == 2)
					{
						Game::RemoveLevel();
						Game::ChangeState(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
					}
				}
				else if(CurrentMenu == MENU_PLAYER_FAILED_NO_ESCAPE)
				{
					if(SelectedItem == 0)
					{
						Game::GetLevel()->Reload();
						SetCurrentTransition(TRANSITION_LEVELSTART);
						Game::ChangeState(STATE_TRANSITION);
						Game::ResetPlayerLives();
					}
					else if(SelectedItem == 1)
					{
						Game::RemoveLevel();
						Game::ChangeState(STATE_MENU);
						SetCurrentMenu(MENU_MAIN);
					}
				}
				break;
			case BIND_BACK: case BIND_ESCAPE:
				if(CurrentMenu == MENU_MAIN)
					Game::SetGameEndFlag();
				if(CurrentMenu == MENU_OPTIONS)
					SetCurrentMenu(MENU_MAIN);
				if(CurrentMenu == MENU_VIDEO_OPTIONS)
					SetCurrentMenu(MENU_OPTIONS);
				if(CurrentMenu == MENU_SOUND_OPTIONS)
					SetCurrentMenu(MENU_OPTIONS);
				if(CurrentMenu == MENU_BINDS)
					SetCurrentMenu(MENU_OPTIONS);
				if(CurrentMenu == MENU_MAPSELECT)
					SetCurrentMenu(MENU_MAIN);
				if(CurrentMenu == MENU_PAUSE)
				{
					Game::ChangeState(STATE_GAME);
					Sound::ResumeMusic();
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
					Sound::SetMusicVolume(currentVolume);
				}
				if(SelectedItem == 1)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume--;
					if(currentVolume < 0)
						currentVolume = 128;
					menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					Sound::SetSfxVolume(currentVolume);
				}
			}
			if(CurrentMenu == MENU_VIDEO_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					menus.at(MENU_SELECTION_DISPLAY)->selected >= (menus.at(MENU_SELECTION_DISPLAY)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_DISPLAY)->selected = 0 : menus.at(MENU_SELECTION_DISPLAY)->selected++;
					Graphics::SetDisplayIndex(menus.at(MENU_SELECTION_DISPLAY)->selected);
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 1)
					menus.at(MENU_SELECTION_DISPLAY_MODE)->selected >= (menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = 0 : menus.at(MENU_SELECTION_DISPLAY_MODE)->selected++;
				else if(SelectedItem == 2)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected <= 0 ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) : menus.at(MENU_SELECTION_FULLSCREEN)->selected--;
					Graphics::SetFullscreenMode(menus.at(MENU_SELECTION_FULLSCREEN)->selected);
					Graphics::UpdateDisplayMode();
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 3)
				{
					menus.at(MENU_SELECTION_SCALING_MODE)->selected <= 0 ? menus.at(MENU_SELECTION_SCALING_MODE)->selected = (menus.at(MENU_SELECTION_SCALING_MODE)->GetItemCount() - 1) : menus.at(MENU_SELECTION_SCALING_MODE)->selected--;
					Graphics::SetScalingMode(menus.at(MENU_SELECTION_SCALING_MODE)->selected);
					Graphics::UpdateDisplayMode();
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
					Sound::SetMusicVolume(currentVolume);
				}
				if(SelectedItem == 1)
				{
					int currentVolume = atoi(menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->text.c_str());
					currentVolume++;
					if(currentVolume > 128)
						currentVolume = 0;
					menus.at(MENU_SELECTION_SFX_VOLUME)->GetItemInfo(0)->SetText(std::to_string(currentVolume));
					Sound::SetSfxVolume(currentVolume);
				}
			}
			if(CurrentMenu == MENU_VIDEO_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					menus.at(MENU_SELECTION_DISPLAY)->selected <= 0 ? menus.at(MENU_SELECTION_DISPLAY)->selected = (menus.at(MENU_SELECTION_DISPLAY)->GetItemCount() - 1) : menus.at(MENU_SELECTION_DISPLAY)->selected--;
					Graphics::SetDisplayIndex(menus.at(MENU_SELECTION_DISPLAY)->selected);
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 1)
					menus.at(MENU_SELECTION_DISPLAY_MODE)->selected <= 0 ? menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = (menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1) : menus.at(MENU_SELECTION_DISPLAY_MODE)->selected--;
				else if(SelectedItem == 2)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected >= (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = 0 : menus.at(MENU_SELECTION_FULLSCREEN)->selected++;
					Graphics::SetFullscreenMode(menus.at(MENU_SELECTION_FULLSCREEN)->selected);
					Graphics::UpdateDisplayMode();
					RefreshDisplayModeMenus();
				}
				else if(SelectedItem == 3)
				{
					menus.at(MENU_SELECTION_SCALING_MODE)->selected >= (menus.at(MENU_SELECTION_SCALING_MODE)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_SCALING_MODE)->selected = 0 : menus.at(MENU_SELECTION_SCALING_MODE)->selected++;
					Graphics::SetScalingMode(menus.at(MENU_SELECTION_SCALING_MODE)->selected);
					Graphics::UpdateDisplayMode();
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
	Sound::PlaySfx("menu-select");
}

MenuItem::~MenuItem()
{
}

MenuItem::MenuItem(int x, int y, std::string text, FONTS font, SDL_Color standardColor, SDL_Color selectedColor, TEXT_ALIGN align)
{
	pos.x = x;
	pos.y = y;
	this->text = text;
	this->font = font;
	this->standardColor = standardColor;
	this->selectedColor = selectedColor;
	this->align = align;
}

MenuItem::MenuItem(SDL_Point pos, std::string text, FONTS font, SDL_Color standardColor, SDL_Color selectedColor)
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
	int centerX = Graphics::GetWindowNormalizedX(0.5);
	int centerY = Graphics::GetWindowNormalizedY(0.5);

	Menu *menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX + 32 * 5, centerY - 32 * 3, "START GAME", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX + 32 * 4, centerY, "SETTINGS", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX + 32 * 2, centerY + 32 * 3, "EXIT", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_MAIN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * 6, "VIDEO", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * 3, "SOUND", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY, "CONTROLS", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 3, "BACK", FONT_MENU, menu_color, selected_color));
	menus[MENU_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * 6, "DISPLAY:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX - 32 * 5, centerY - 32 * 3, "MODE:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX, centerY, "FULLSCREEN:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 3, "SCALING:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 6, "BACK", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_VIDEO_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * 6, "MUSIC:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX - 32 * 2, centerY - 32 * 3, "SFX:", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menu->AddMenuItem(new MenuItem(centerX, centerY, "BACK", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_SOUND_OPTIONS] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY - 60, "RESUME", FONT_GAME, pause_color, selected_color, TEXT_ALIGN_CENTER));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 60, "QUIT", FONT_GAME, pause_color, selected_color, TEXT_ALIGN_CENTER));
	menus[MENU_PAUSE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX + 32 * 6, centerY - 32 * 6, std::to_string(Sound::GetMusicVolume()), FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_SELECTION_MUSIC_VOLUME] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX + 32 * 6, centerY - 32 * 3, std::to_string(Sound::GetSfxVolume()), FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
	menus[MENU_SELECTION_SFX_VOLUME] = menu;

	menu = new Menu();
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY] = menu;

	menu = new Menu();
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_DISPLAY_MODE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY, "DISABLED", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY, "ENABLED", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY, "BORDERLESS", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_FULLSCREEN] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY + 32 * 3, "DEFAULT", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY + 32 * 3, "ADAPTIVE", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->AddMenuItem(new MenuItem(centerX + 32, centerY + 32 * 3, "LETTERBOXED", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	menu->IsSwitchable = true;
	menus[MENU_SELECTION_SCALING_MODE] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY, "RETRY LEVEL", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 3, "NEW LEVEL", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 6, "BACK TO MENU", FONT_MENU, menu_color, selected_color));
	menus[MENU_PLAYER_FAILED] = menu;

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 3, "NEW LEVEL", FONT_MENU, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(centerX, centerY + 32 * 6, "BACK TO MENU", FONT_MENU, menu_color, selected_color));
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
		int x = Graphics::GetWindowNormalizedX(0.3) - 32;
		int y = centerY - 32 * (half - i) * 2;
		menu->AddMenuItem(new MenuItem(x, y, GetBindingName(bind), FONT_MENU, menu_color, selected_color, TEXT_ALIGN_RIGHT));
		i++;
	}

	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * (half - GetNumConfigurableBinds()) * 2, "RESET TO DEFAULT", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_CENTER));
	menu->AddMenuItem(new MenuItem(centerX, centerY - 32 * (half - GetNumConfigurableBinds() - 1) * 2, "BACK", FONT_MENU, menu_color, selected_color, TEXT_ALIGN_CENTER));
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
	std::map<int, std::vector<SDL_DisplayMode>> displayModes = Graphics::GetDisplayModes();
	for(auto display : displayModes)
	{
		std::ostringstream modeName;
		// TODO: Proper UTF-8 text rendering
		//modeName << SDL_GetDisplayName(display.first);
		modeName << display.first;
		menus.at(MENU_SELECTION_DISPLAY)->AddMenuItem(new MenuItem(Graphics::GetWindowNormalizedX(0.5) + 32, Graphics::GetWindowNormalizedY(0.5) - 32 * 6, modeName.str(), FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	}
	return 0;
}

std::vector<std::string> GetAllTiledMaps()
{
	std::vector<std::string> names;
	GetFolderFileList("assets/levels", names);
	for(auto i = names.begin(); i != names.end();)
	{
		if(!HasEnding(*i, ".tmx"))
			i = names.erase(i);
		else
			++i;
	}
	return names;
}

int CreateMapSelectMenu()
{
	std::vector<std::string> maps = GetAllTiledMaps();
	for(int i = 0; i < (int)maps.size(); i++)
	{
		menus.at(MENU_MAPSELECT)->AddMenuItem(new MenuItem(Graphics::GetWindowNormalizedX(0.2), Graphics::GetWindowNormalizedY(0.2) + i * 50, maps[i], FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
	}
	return 0;
}

int RefreshDisplayModeMenus()
{
	delete menus.at(MENU_SELECTION_DISPLAY_MODE);
	Menu *menu = new Menu();
	menu->IsSwitchable = true;
	menus.at(MENU_SELECTION_DISPLAY_MODE) = menu;

	SDL_DisplayMode currentMode = Graphics::GetDisplayMode();
	std::map<int, std::vector<SDL_DisplayMode>> displayModes = Graphics::GetDisplayModes();
	for(auto mode : displayModes[Graphics::GetDisplayIndex()])
	{
		std::ostringstream modeName;
		modeName << mode.w << "x" << mode.h << "@" << mode.refresh_rate << "HZ " << SDL_BITSPERPIXEL(mode.format) << "-BIT";
		menus.at(MENU_SELECTION_DISPLAY_MODE)->AddMenuItem(new MenuItem(Graphics::GetWindowNormalizedX(0.5) - 32 * 4, Graphics::GetWindowNormalizedY(0.5) - 32 * 3, modeName.str(), FONT_MENU, menu_color, selected_color, TEXT_ALIGN_LEFT));
		
		if(currentMode.w == mode.w &&
			currentMode.h == mode.h &&
			currentMode.refresh_rate == mode.refresh_rate &&
			currentMode.format == mode.format)
			menus.at(MENU_SELECTION_DISPLAY_MODE)->selected = menus.at(MENU_SELECTION_DISPLAY_MODE)->GetItemCount() - 1;
	}

	menus.at(MENU_SELECTION_FULLSCREEN)->selected = Graphics::GetFullscreenMode();
	menus.at(MENU_SELECTION_SCALING_MODE)->selected = Graphics::GetScalingMode();
	menus.at(MENU_SELECTION_DISPLAY)->selected = Graphics::GetDisplayIndex();
	return 0;
}

KEYBINDS GetCurrentKeyToBind()
{
	return BindingKey;
}

std::map<MENUS, Menu*>* GetMenus()
{
	return &menus;
}