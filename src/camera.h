#ifndef _camera_h_
#define _camera_h_ 

#include <SDL.h>
#include "entities.h"

class Camera
{
	private:
		double x;
		double y;
		double h;
		double w;
		int offsetX;
		int offsetY;
		double factorX = 0.045;
		double factorY = 0.055;
		// attached to what
		Entity *at;
		// used for limiting camera movement
		SDL_Rect virtualCam;

	public:
		Camera();
		Camera(double x, double y, double w, double h);

		~Camera();
		void Attach(Entity &p);
		void Detach();
		void Update();
		SDL_Rect GetRect();
		void SetRect(SDL_Rect &r);
		PrecisionRect GetPRect();
		void SetOffsetX(int x);
		void SetOffsetY(int y);
		bool IsAttachedTo(Entity *e);
		SDL_Rect GetVirtualCamRect();
};

#endif
