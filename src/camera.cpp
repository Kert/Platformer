#include "camera.h"
#include "graphics.h"
#include "level.h"
#include "utils.h"

extern int map_width;
extern int map_height;
extern Level *level;

Camera::Camera()
{
	x = 0;
	y = 0;
	h = 0;
	w = 0;
	at = nullptr;
	offsetX = 0;
	offsetY = 0;
}

Camera::~Camera()
{
	
}

Camera::Camera(double x, double y, double w, double h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	at = nullptr;
	offsetX = 0;
	offsetY = 10;
}

SDL_Rect Camera::GetRect()
{
	SDL_Rect r;
	r.h = (int)round(h);
	r.w = (int)round(w);
	r.x = (int)round(x);
	r.y = (int)round(y);
	return r;
}

PrecisionRect Camera::GetPRect()
{
	return { x, y, h, w };
}

void Camera::SetRect(SDL_Rect &r)
{	
	h = r.h;
	w = r.w;
	x = r.x;
	y = r.y;
}

void Camera::Attach(Entity &p)
{
	at = &p;
	SetOffsetX(0);
	SetOffsetY(0);
	x = at->hitbox->GetPRect().x - WIDTH / RENDER_SCALE / 2;
	y = at->hitbox->GetPRect().y - HEIGHT / RENDER_SCALE / 2;
}

void Camera::Detach()
{
	at = nullptr;	
}

void Camera::SetOffsetX(int x)
{
	//offsetX = x;
}

void Camera::SetOffsetY(int y)
{
	//offsetY = y;
}

void Camera::Update()
{
	if(at != nullptr)
	{
		//Center the camera over the player
		//x = ( at->hitbox->GetPRect().x + at->hitbox->GetPRect().w / 2. ) - WIDTH / RENDER_SCALE / 2.;
		//x = at->hitbox->GetPRect().x - WIDTH / RENDER_SCALE / 2.;
		//y = at->hitbox->GetPRect().y - HEIGHT / RENDER_SCALE / 2.;// - 26 - 32 - HEIGHT / RENDER_SCALE / 2.;

		//double val;
		//double diff;
		//val = (at->hitbox->GetRect().x + at->hitbox->GetRect().w / 2) - WIDTH / RENDER_SCALE / 2;
		//val += offsetX;
		//diff = val - x;
		//if (abs(diff) > 1)
		//{
		//	double tmp;
		//	tmp = abs(diff);
		//	if (diff < 0)
		//		x = x - pow(tmp, 0.5);
		//		//x = x - 1.5 * pow(0.99999, tmp); // expo
		//	else
		//		x = x + pow(tmp, 0.5);
		//		//x = x + 1.5 * pow(0.99999, tmp); // expo
		//}
		//	
		//val = (at->hitbox->GetRect().y + at->hitbox->GetRect().h / 2) - HEIGHT / RENDER_SCALE / 2;
		//val += offsetY;
		//diff = val - y;
		//if (abs(diff) > 1)
		//	y = y + factorY * (diff); // linear interpolation
		
		std::vector<SDL_Rect> activeBounds;
		for(auto i : level->CameraBounds)
		{
			if(SDL_HasIntersection(&at->hitbox->GetRect(), &i))
				activeBounds.push_back(i);
		}

		double playerX = at->hitbox->GetRect().x;
		double playerY = at->hitbox->GetRect().y;

		if(activeBounds.size() == 1)
		{
			SDL_Rect i = activeBounds[0];
			currentBounds = i;

			x = playerX - WIDTH / RENDER_SCALE / 2;
			if(x < i.x)
				x = i.x;
			if(x + WIDTH / RENDER_SCALE > i.x + i.w)
				x = i.x + i.w - WIDTH / RENDER_SCALE;
			
			y = playerY - HEIGHT / RENDER_SCALE / 2;
			if(y < i.y)
				y = i.y;
			if(y + HEIGHT / RENDER_SCALE > i.y + i.h)
				y = i.y + i.h - HEIGHT / RENDER_SCALE;
		}
		else if(activeBounds.size() > 1)
		{
			for(auto i : activeBounds)
			{
				SDL_Rect res;
				if(SDL_IntersectRect(&at->hitbox->GetRect(), &i, &res))
				{
					bool vertical = false;
					if(i.h > i.w)
						vertical = true;
					SDL_Rect cam = this->GetRect();
					SDL_IntersectRect(&cam, &i, &res);

					// if player hitbox farther than currentCamX
					if(at->hitbox->GetRect().x > (x + WIDTH / RENDER_SCALE / 2))
					{
						double newX = x + 1; //at->hitbox->GetRect().x - (x + WIDTH / RENDER_SCALE / 2);
						if(newX <= i.x + i.w - WIDTH / RENDER_SCALE)
							if(!vertical && SDL_RectEquals(&cam, &res))
								//x = newX;
								x = playerX - WIDTH / RENDER_SCALE / 2;
					}
					// if player hitbox closer than currentCamX
					else if(at->hitbox->GetRect().x < (x + WIDTH / RENDER_SCALE / 2))
					{
						double newX = x - 1; // +at->hitbox->GetRect().x - (x + WIDTH / RENDER_SCALE / 2);
						if(newX >= i.x)
							if(!vertical && SDL_RectEquals(&cam, &res))
								//x = newX;
								x = playerX - WIDTH / RENDER_SCALE / 2;
					}
					// if player hitbox lower than currentCamY
					if(at->hitbox->GetRect().y > (y + HEIGHT / RENDER_SCALE / 2))
					{
						double newY = y + 1; // at->hitbox->GetRect().y - (y + HEIGHT / RENDER_SCALE / 2);
						if(newY <= i.y + i.h - HEIGHT / RENDER_SCALE)
							if(vertical && SDL_RectEquals(&cam, &res))
							{
								PrintLog(LOG_SUPERDEBUG, "2 MOVE LOWER BECAUSE HITBOX THERE!");
								//y = newY;
								y = playerY - HEIGHT / RENDER_SCALE / 2;
							}
					}
					// if player hitbox higher than currentCamY
					else if(at->hitbox->GetRect().y < (y + HEIGHT / RENDER_SCALE / 2))
					{
						double newY = y - 1; //at->hitbox->GetRect().y - (y + HEIGHT / RENDER_SCALE / 2);
						if(newY >= i.y)
							if(vertical && SDL_RectEquals(&cam, &res))
							{
								PrintLog(LOG_SUPERDEBUG, "2 MOVE HIGHER BECAUSE HITBOX THERE!");
								//y = newY;
								y = playerY - HEIGHT / RENDER_SCALE / 2;
							}
					}
				}
			}
		}
		else
		{
			x = playerX - WIDTH / RENDER_SCALE / 2;
			SDL_Rect i = currentBounds;
			if(x < i.x)
				x = i.x;
			if(x + WIDTH / RENDER_SCALE > i.x + i.w)
				x = i.x + i.w - WIDTH / RENDER_SCALE;
		}
		

		//Keep the camera in bounds.
		if(x < 0 )
		{
			x = 0;
		}
		if(y < 0)
		{
	 		y = 0;
		}
		if(x > map_width - w)
		{
			x = map_width - w;
		}
		if(y > map_height - h)
		{
			y = map_height - h;
		}
	}
	else {}
}

bool Camera::IsAttachedTo(Entity *e)
{
	return e == this->at;
}