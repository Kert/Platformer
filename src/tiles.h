#ifndef _tiles_h_
#define _tiles_h_ 

#include <SDL.h>
#include <string>
//#include <vld.h>
#include "globals.h"

void LoadTileSet();
void AddDataToTileSet(int type, int x_offset, int y_offset);
void TilesCleanup();
void DeleteAllTiles();
PHYSICS_TYPES GetTileTypeAtTiledPos(int x, int y);
PHYSICS_TYPES GetTileTypeAtTiledPos(SDL_Point at);
PHYSICS_TYPES GetTileTypeAtPos(int x, int y);
PHYSICS_TYPES GetTileTypeAtPos(SDL_Point at);

class Tile
{
	public:
		char type; // collision type
		// coords on map (tiled coords)
		int x;
		int y;
		// coords in texture
		int tex_x;
		int tex_y;
		// layer number
		int layer;
		// unique tile
		CustomTile *customTile = nullptr;
		// animation stuff
		TileAnimationData *animation = nullptr;
		// texture surface
		SDL_Surface *src_tex;
		Tile(int x, int y, int layer, CustomTile *data, bool replace);
		Tile(int x, int y, int layer, CustomTile *data, char type, bool replace);
		int GetID();
		bool HasAnimation();
		void Animate();
		~Tile();
};

#endif
