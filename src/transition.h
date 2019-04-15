#ifndef _transition_h_
#define _transition_h_

enum TRANSITIONS
{
	TRANSITION_TITLE,
	TRANSITION_LEVELSTART,
	TRANSITION_LEVELCLEAR
};

void ProgressTransition();
TRANSITIONS GetCurrentTransition();
void SetCurrentTransition(TRANSITIONS transition);

#endif
