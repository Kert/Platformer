#ifndef _sprite_h_
#define _sprite_h_ 

#include <SDL.h>
#include <map>
#include <vector>
#include "animation.h"
#include "globals.h"

class Sprite
{
	private:
		SDL_Rect rect;
		std::map<ANIMATION_TYPE, Animation> animation;
		ANIMATION_TYPE current_anim;
		ANIMATION_TYPE last_anim;
		SDL_Texture **sprite_sheet;

		int offset_x;
		int offset_y;

	public:
		int shootingAnimTimer = 0;
		//Initializes the variables
		Sprite() {};
		~Sprite();

		Sprite(SDL_Texture **tex, SDL_Rect rect);
		Sprite(SDL_Texture **tex, int x, int y, int h, int w);
		SDL_Rect GetTextureCoords();
		void AddAnimation(ANIMATION_TYPE type, int offset_x, int offset_y, int frames, int interval, int fps, ANIM_LOOP_TYPES loop);
		void SetAnimation(ANIMATION_TYPE type);
		bool AnimationExists(ANIMATION_TYPE type);
		void StopAnimation();
		ANIMATION_TYPE GetAnimation();
		SDL_Texture* GetSpriteSheet();
		void SetSpriteOffset(int x, int y);
		int GetSpriteOffsetX();
		int GetSpriteOffsetY();
		void SetSpriteRect(SDL_Rect rect);
		void SetSpriteTexture(SDL_Texture **tex);
		void SetCurrentFrame(int frame);
		void Animate();
		void SetSpriteSize(int width, int height);
		void SetSpriteY(int y);
};

#endif
