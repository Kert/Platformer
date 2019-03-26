#include "camera.h"
#include "graphics.h"
#include "level.h"
#include "utils.h"

extern int map_width;
extern int map_height;
extern Level *level;

extern int GAME_SCENE_WIDTH;
extern int GAME_SCENE_HEIGHT;

const int VIRTUAL_CAM_WIDTH = 22 * TILESIZE;
const int VIRTUAL_CAM_HEIGHT = 18 * TILESIZE;

Camera::Camera()
{
	x = 0;
	y = 0;
	h = 0;
	w = 0;
	at = nullptr;
	offsetX = 0;
	offsetY = 0;
	virtualCam.x = virtualCam.y = 0;
	virtualCam.w = VIRTUAL_CAM_WIDTH;
	virtualCam.h = VIRTUAL_CAM_HEIGHT;
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
	virtualCam.x = virtualCam.y = 0;
	virtualCam.w = VIRTUAL_CAM_WIDTH;
	virtualCam.h = VIRTUAL_CAM_HEIGHT;
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
	return{ x, y, h, w };
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
	x = at->hitbox->GetPRect().x - GAME_SCENE_WIDTH / 2;
	y = at->hitbox->GetPRect().y - GAME_SCENE_HEIGHT / 2;
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
		double playerX = at->hitbox->GetRect().x;
		double playerY = at->hitbox->GetRect().y + at->hitbox->GetRect().h;

		SDL_Rect newRectX, newRectY;
		newRectX.x = playerX - VIRTUAL_CAM_WIDTH / 2;
		newRectX.y = virtualCam.y;
		newRectX.w = virtualCam.w;
		newRectX.h = virtualCam.h;

		newRectY.x = virtualCam.x;
		newRectY.y = playerY - VIRTUAL_CAM_HEIGHT / 2;
		newRectY.w = virtualCam.w;
		newRectY.h = virtualCam.h;

		bool xFine, yFine;
		xFine = yFine = false;

		for(auto bound : level->CameraBounds)
		{
			SDL_Rect intersectX;
			SDL_IntersectRect(&bound,&newRectX,&intersectX);
			SDL_Rect intersectY;
			SDL_IntersectRect(&bound, &newRectY, &intersectY);
			if(SDL_RectEquals(&intersectX, &newRectX))
				xFine = true;
			if(SDL_RectEquals(&intersectY, &newRectY))
				yFine = true;
		}

		if(xFine)
		{
			virtualCam.x = newRectX.x;
			x = playerX - GAME_SCENE_WIDTH / 2;
		}
		if(yFine)
		{
			virtualCam.y = newRectY.y;
			y = playerY - GAME_SCENE_HEIGHT / 2;
		}

		//Keep the camera in bounds.
		if(x < 0)
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