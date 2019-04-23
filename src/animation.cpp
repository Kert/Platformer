#include "animation.h"
#include <SDL.h>
#include "utils.h"

Animation::Animation()
{
	CurrentFrame = 1;
	Frames = 1;
	FrameInc = 1;
	FrameRate = 100; //Milliseconds
	OldTime = SDL_GetTicks();
	PixelInterval = 16;
	OffsetX = 0;
	OffsetY = 0;
	loop = ANIM_LOOP_TYPES::LOOP_NONE;
	loopFromFrame = 1;
}

Animation::Animation(int offset_x, int offset_y, int frames, int interval, int fps, ANIM_LOOP_TYPES loop, int loopFrom)
{
	CurrentFrame = 1;
	FrameInc = 1;
	OldTime = SDL_GetTicks();
	OffsetX = offset_x;
	OffsetY = offset_y;
	Frames = frames;
	PixelInterval = interval;
	FrameRate = fps;
	this->loop = loop;
	loopFromFrame = loopFrom;
}

void Animation::Animate(int &x, int &y)
{
	if(OldTime + FrameRate > SDL_GetTicks())
	{
		return;
	}

	//PrintLog(LOG_INFO, "current frame %i", CurrentFrame);
	OldTime = SDL_GetTicks();
	if(loop == ANIM_LOOP_TYPES::LOOP_PINGPONG)
	{
		if(FrameInc > 0)
		{
			if(CurrentFrame >= Frames)
			{
				FrameInc = -FrameInc;
			}
		}
		else
		{
			if(CurrentFrame <= loopFromFrame)
			{
				FrameInc = -FrameInc;
			}
		}
	}
	else if(loop == ANIM_LOOP_TYPES::LOOP_NORMAL)
	{
		if(CurrentFrame > Frames)
		{
			CurrentFrame = loopFromFrame;
		}
	}
	else if(loop == ANIM_LOOP_TYPES::LOOP_NONE)
	{
		if(CurrentFrame > Frames)
		{
			// Stop on the last frame
			CurrentFrame--;
		}
	}
	x = OffsetX + PixelInterval * (CurrentFrame - 1);
	y = OffsetY;
	CurrentFrame += FrameInc;
}

void Animation::SetFrameRate(int Rate)
{
	FrameRate = Rate;
}

void Animation::SetCurrentFrame(int Frame)
{
	if(Frame < 0 || Frame >= Frames) return;

	CurrentFrame = Frame;
}

int Animation::GetCurrentFrame()
{
	return CurrentFrame;
}

void Animation::ShowFrame(int Frame, int &x, int &y)
{
	x = OffsetX + PixelInterval * (Frame - 1);
	y = OffsetY;
}