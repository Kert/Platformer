#ifndef _transition_h_
#define _transition_h_

enum TRANSITIONS
{
	TRANSITION_TITLE,
	TRANSITION_LEVELSTART,
	TRANSITION_LEVELCLEAR,
	TRANSITION_LEVELLOSE,
	TRANSITION_GAMEOVER
};

void ProgressTransition();
void SetCurrentTransition(TRANSITIONS transition);

#endif
