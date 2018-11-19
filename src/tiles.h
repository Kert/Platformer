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
void LoadTileTypesFromFile(std::string filename);
PHYSICS_TYPES GetTileTypeAtTiledPos(int x, int y);
PHYSICS_TYPES GetTileTypeAtTiledPos(SDL_Point at);
PHYSICS_TYPES GetTileTypeAtPos(int x, int y);
PHYSICS_TYPES GetTileTypeAtPos(SDL_Point at);
bool IsSolid(PHYSICS_TYPES type);

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
		// unique tile
		CustomTile *customTile = nullptr;
		// animation stuff
		TileAnimationData *animation = nullptr;
		// texture surface
		SDL_Surface *src_tex;
		Tile(int x, int y, CustomTile *data, bool replace, TILEMAP_LAYERS layer = LAYER_BACKGROUND);
		Tile(int x, int y, CustomTile *data, char type, bool replace, TILEMAP_LAYERS layer = LAYER_BACKGROUND);
		int GetID();
		bool HasAnimation();
		~Tile();
};

void UnblitTile(Tile *tile);

#endif
