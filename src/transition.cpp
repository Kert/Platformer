#include "transition.h"
#include <SDL.h>
#include "gamelogic.h"
#include "graphics.h"
#include "level.h"
#include "menu.h"
#include "input.h"

TRANSITIONS TransitionID;

void ProgressTransition()
{
	switch(TransitionID)
	{
		case TRANSITION_TITLE:
			Game::ChangeState(STATE_MENU);
			SetCurrentMenu(MENU_MAIN);
			break;
		case TRANSITION_LEVELCLEAR:
			Game::RemoveLevel();
			// Load next level
			//Game::ChangeState(STATE_GAME);
			// Or load level select screen
			Game::ChangeState(STATE_MENU);
			SetCurrentMenu(MENU_MAPSELECT);			
			break;
	}
}

TRANSITIONS GetCurrentTransition()
{
	return TransitionID;
}

void SetCurrentTransition(TRANSITIONS transition)
{
	TransitionID = transition;
}
