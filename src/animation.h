#ifndef _animation_h_
#define _animation_h_

#include <SDL.h>
#include "globals.h"

enum ANIMATION_TYPE
{
	ANIMATION_NONE,
	ANIMATION_RUNNING,
	ANIMATION_STANDING,
	ANIMATION_JUMPING,
	ANIMATION_FALLING,
	ANIMATION_HANGING,
	ANIMATION_SLIDING,
	ANIMATION_DUCKING,
	ANIMATION_DYING,
	ANIMATION_LOOKINGUP,
	ANIMATION_SHOOTING,
	ANIMATION_SHOOTING_STANDING, // ok
	ANIMATION_SHOOTING_HANGING, // ok
	ANIMATION_SHOOTING_JUMPING, // ok
	ANIMATION_SHOOTING_FALLING, // ok
	ANIMATION_SHOOTING_RUNNING,
	ANIMATION_IDLE
};

class Animation {
	private:
		int    CurrentFrame;
		int     FrameInc;
		int     FrameRate; //Milliseconds
		int PixelInterval;
		int OffsetX;
		int OffsetY;
		Uint32    OldTime;
		int loopFromFrame;

	public:
		int    Frames;
		ANIM_LOOP_TYPES   loop;

	public:
		Animation();
		Animation(int offset_x, int offset_y, int frames, int interval, int fps, ANIM_LOOP_TYPES loop, int loopFrom);
		void Animate(int &x, int &y);

	public:
		void SetFrameRate(int Rate);
		void SetCurrentFrame(int Frame);
		void ShowFrame(int Frame, int &x, int &y);
		int GetCurrentFrame();
};

#endif
