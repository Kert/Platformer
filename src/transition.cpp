#include "transition.h"
#include <SDL.h>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "menu.h"
#include "input.h"

int TransitionID;

extern Level *level;

void ProgressTransition()
{
	switch(TransitionID)
	{
		case TRANSITION_TITLE:
			Game::ChangeState(STATE_MENU);
			SetCurrentMenu(MENU_MAIN);
			break;
		case TRANSITION_LEVELSTART:
			if(level->loaded)
			{
				Game::ChangeState(STATE_GAME);
			}
			break;
		case TRANSITION_LEVELCLEAR:
			SetCurrentTransition(TRANSITION_LEVELSTART);
			Game::ChangeState(STATE_TRANSITION);
			// force the loading screen to draw for one frame before we start loading
			Graphics::RenderTransition();
			Graphics::WindowUpdate();
			delete level;
			level = new Level();
			break;
	}
}

void SetCurrentTransition(TRANSITIONS transition)
{
	TransitionID = transition;
}
