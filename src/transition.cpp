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
			SetCurrentTransition(TRANSITION_LEVELSTART);
			Game::ChangeState(STATE_TRANSITION);
			// force the loading screen to draw for one frame before we start loading
			Graphics::RenderTransition();
			Graphics::WindowUpdate();
			Game::RemoveLevel();
			Game::GetLevel()->Reload();
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
