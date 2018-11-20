#include "sprite.h"
#include "utils.h"

Sprite::~Sprite() {}

Sprite::Sprite(SDL_Texture **tex, SDL_Rect rect)
{
	this->rect = rect;
	sprite_sheet = tex;
	current_anim = ANIMATION_NONE;
	SetSpriteOffset(0, 0);
}

Sprite::Sprite(SDL_Texture **tex, int x, int y, int h, int w)
{
	this->rect.x = x;
	this->rect.y = y;
	this->rect.h = h;
	this->rect.w = w;
	sprite_sheet = tex;
	current_anim = ANIMATION_NONE;
	SetSpriteOffset(0, 0);
}

SDL_Rect Sprite::GetTextureCoords()
{
	return rect;
}

void Sprite::SetAnimation(ANIMATION_TYPE type)
{
	// fallbacks if animation isn't defined
	if(type == ANIMATION_FALLING && !AnimationExists(ANIMATION_FALLING))
		type = ANIMATION_JUMPING;
	if(!AnimationExists(type))
		type = ANIMATION_STANDING;

	if(last_anim != type)
	{
		current_anim = type;
		PrintLog(LOG_INFO, "Set animation to %i. Old was %i", type, last_anim);
		animation.at(type).SetCurrentFrame(1);
		this->Animate();
		last_anim = type;
	}

}

bool Sprite::AnimationExists(ANIMATION_TYPE type)
{
	return (animation.find(type) != animation.end());
}

void Sprite::Animate()
{
	if(current_anim != ANIMATION_NONE)
		animation.at(current_anim).Animate(rect.x, rect.y);
}

void Sprite::SetCurrentFrame(int frame)
{
	animation.at(current_anim).Animate(rect.x, rect.y);
}

ANIMATION_TYPE Sprite::GetAnimation()
{
	return last_anim;
}

void Sprite::AddAnimation(ANIMATION_TYPE type, int offset_x, int offset_y, int frames, int interval, int fps, ANIM_LOOP_TYPES loop)
{
	animation[type] = Animation(offset_x, offset_y, frames, interval, fps, loop);
}

void Sprite::StopAnimation()
{
	if(current_anim != ANIMATION_NONE)
	{
		animation.at(current_anim).ShowFrame(-1, rect.x, rect.y);
		current_anim = ANIMATION_NONE;
	}
}

SDL_Texture* Sprite::GetSpriteSheet()
{
	return *sprite_sheet;
}

void Sprite::SetSpriteOffset(int x, int y)
{
	offset_x = x;
	offset_y = y;
}

int Sprite::GetSpriteOffsetX()
{
	return offset_x;
}

int Sprite::GetSpriteOffsetY()
{
	return offset_y;
}

void Sprite::SetSpriteSize(int width, int height)
{
	this->rect.w = width;
	this->rect.h = height;
}

void Sprite::SetSpriteY(int y)
{
	this->rect.y = y;
}

void Sprite::SetSpriteRect(SDL_Rect rect)
{
	this->rect = rect;
}

void Sprite::SetSpriteTexture(SDL_Texture **tex)
{
	sprite_sheet = tex;
}
