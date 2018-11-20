#include "menu.h"
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
extern bool ENDGAME;
extern TTF_Font *minor_font;
extern TTF_Font *menu_font;
extern TTF_Font *game_font;
extern SDL_Color menu_color;
extern SDL_Color selected_color;
extern SDL_Color pause_color;

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
						UpdateWindow();
						level = new Level();
					}
					if(SelectedItem == 1)
					{
						SetCurrentMenu(MENU_OPTIONS);
					}
					if(SelectedItem == 2)
						ENDGAME = true;
				}
				else if(CurrentMenu == MENU_OPTIONS)
				{
					if(SelectedItem == 2)
						SetCurrentMenu(MENU_BINDS);
					if(SelectedItem == 3)
					{
						SetCurrentMenu(MENU_MAIN);
					}
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
				{
					menus.at(MENU_SELECTION_LIVES)->selected <= 0 ? menus.at(MENU_SELECTION_LIVES)->selected = (menus.at(MENU_SELECTION_LIVES)->GetItemCount() - 1) : menus.at(MENU_SELECTION_LIVES)->selected--;
				}
				else if(SelectedItem == 1)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected <= 0 ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) : menus.at(MENU_SELECTION_FULLSCREEN)->selected--;
				}
			}
			break;
		case BIND_RIGHT: case BIND_ARROWR:
			if(CurrentMenu == MENU_OPTIONS)
			{
				if(SelectedItem == 0)
				{
					menus.at(MENU_SELECTION_LIVES)->selected >= (menus.at(MENU_SELECTION_LIVES)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_LIVES)->selected = 0 : menus.at(MENU_SELECTION_LIVES)->selected++;
				}
				else if(SelectedItem == 1)
				{
					menus.at(MENU_SELECTION_FULLSCREEN)->selected >= (menus.at(MENU_SELECTION_FULLSCREEN)->GetItemCount() - 1) ? menus.at(MENU_SELECTION_FULLSCREEN)->selected = 0 : menus.at(MENU_SELECTION_FULLSCREEN)->selected++;
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
	menu->AddMenuItem(new MenuItem(315, 150, "Lives: ", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(315, 230, "Fullscreen:", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(315, 310, "Keybinds", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(315, 390, "Back", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(WIDTH / 2, (HEIGHT / 2) - 100, "Resume", game_font, pause_color, selected_color));
	menu->AddMenuItem(new MenuItem(WIDTH / 2, (HEIGHT / 2), "Quit", game_font, pause_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	for(int i = 0; i < MAX_LIVES; i++)
	{
		menu->AddMenuItem(new MenuItem(420 + 30 * i, 150, std::to_string(i + 1), menu_font, menu_color, selected_color));
	}
	menu->IsHorizontal = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(500, 230, "Off", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(570, 230, "On", menu_font, menu_color, selected_color));
#ifdef ALLOW_BORDERLESS
	menu->AddMenuItem(new MenuItem(640, 230, "Borderless", menu_font, menu_color, selected_color));
#endif
	menu->IsHorizontal = true;
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(WIDTH / 2, HEIGHT / 2 + 50, "Retry Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(WIDTH / 2, HEIGHT / 2 + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(WIDTH / 2, HEIGHT / 2 + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus.push_back(menu);

	menu = new Menu();
	menu->AddMenuItem(new MenuItem(WIDTH / 2, HEIGHT / 2 + 100, "New Level", menu_font, menu_color, selected_color));
	menu->AddMenuItem(new MenuItem(WIDTH / 2, HEIGHT / 2 + 150, "Back to Menu", menu_font, menu_color, selected_color));
	menus.push_back(menu);
	// insert bind and bindings menu here

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
		menus.at(MENU_SELECTION_FULLSCREEN)->selected = fullscreenMode;
	}
	if(oldmenu == MENU_OPTIONS)
	{
		playerLives = menus.at(MENU_SELECTION_LIVES)->selected + 1;
		fullscreenMode = menus.at(MENU_SELECTION_FULLSCREEN)->selected;
		UpdateWindowMode();
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
