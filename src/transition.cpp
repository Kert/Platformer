#include "transition.h"
#include <SDL.h>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "menu.h"
#include "input.h"

int TransitionID;

extern int playerLives;
extern int currentLives;
extern int SelectedItem;
extern Level *level;

void ProgressTransition()
{
	switch(TransitionID)
	{
		case TRANSITION_TITLE:
			ChangeGamestate(STATE_MENU);
			SetCurrentMenu(MENU_MAIN);
			break;
		case TRANSITION_LEVELSTART:
			if(level->loaded)
			{
				ChangeGamestate(STATE_GAME);
			}
			break;
		case TRANSITION_LEVELCLEAR:
			SetCurrentTransition(TRANSITION_LEVELSTART);
			ChangeGamestate(STATE_TRANSITION);
			// force the loading screen to draw for one frame before we start loading
			RenderTransition();
			WindowUpdate();
			delete level;
			level = new Level();
			break;
		case TRANSITION_LEVELLOSE:
		{
			int offset = currentLives < 1 ? 1 : 0;
			if(SelectedItem + offset == 0)
			{
				level->Reload();
				SetCurrentTransition(TRANSITION_LEVELSTART);
			}
			else if(SelectedItem + offset == 1)
			{
				SetCurrentTransition(TRANSITION_LEVELSTART);
				if(offset == 1) currentLives = playerLives;
				// force the loading screen to draw for one frame before we start loading
				RenderTransition();
				WindowUpdate();
				level->Reload();
			}
			else if(SelectedItem + offset == 2)
			{
				delete level;
				level = nullptr;
				ChangeGamestate(STATE_MENU);
				SetCurrentMenu(MENU_MAIN);
			}
			break;
		}
	}
}

void SetCurrentTransition(TRANSITIONS transition)
{
	TransitionID = transition;
}
