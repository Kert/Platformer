#include "tiles.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "level.h"
#include "utils.h"

extern std::vector<std::vector<int>> tiles;

extern SDL_Surface *surface_level_textures;
extern SDL_Surface *surface_fg;
extern SDL_Texture *level_fgLayer_tex;

extern Level *level;

std::vector<std::vector<Tile*>> tilemap_bg;
std::vector<std::vector<Tile*>> tilemap_fg;

std::vector<CustomTile> tileset;


void LoadTileSet()
{
	int width, height;
	width = surface_level_textures->w / TILESIZE;
	height = surface_level_textures->h / TILESIZE;
	for(int i = 0; i < width * height; i++)
	{
		// Calculating tile ID
		int rem = i % width; // remainder
		int row = i / width;
		AddDataToTileSet(PHYSICS_AIR, rem * TILESIZE, row * TILESIZE);
	}
}

void AddDataToTileSet(int type, int x_offset, int y_offset)
{
	CustomTile c = { type, x_offset, y_offset };
	tileset.push_back(c);
}

void DeleteAllTiles()
{
	for(auto &i : tilemap_bg)
	{
		for(auto &j : i)
		{
			if(j)
			{
				delete j;
				j = nullptr;
			}
		}
	}
	for(auto &i : tilemap_fg)
	{
		for(auto &j : i)
		{
			if(j)
			{
				delete j;
				j = nullptr;
			}
		}
	}
	tilemap_bg.clear();
	tilemap_fg.clear();
}

void LoadTileTypesFromFile(std::string filename)
{
	std::ifstream infile(filename);
	std::string line;

	int i = 0;
	while(std::getline(infile, line))
	{
		std::stringstream iss(line);
		size_t pos = 0;
		std::string delimiter = "\t";
		pos = line.find(delimiter);
		while(pos != std::string::npos)
		{
			pos = line.find('\t');
			std::string token;
			token = line.substr(0, pos);
			std::stringstream str(token);
			int val;
			str >> val;
			tileset[i].type = val;
			i++;
			if(pos == std::string::npos)
				line.erase(0, line.size());
			else
				line.erase(0, pos + delimiter.size());
		}
	}
}

Tile::Tile(int x, int y, CustomTile *data, bool replace, TILEMAP_LAYERS layer)
{
	this->x = x;
	this->y = y;

	if(x >= (int)tilemap_bg.size() || y >= (int)tilemap_bg[0].size())
	{
		PrintLog(LOG_IMPORTANT, "Attempted to place a tile outside of level boundaries: %d %d", x, y);
		delete this;
		return;
	}
	else
	{
		this->type = data->type;
		this->tex_x = data->x_offset;
		this->tex_y = data->y_offset;
		src_tex = surface_level_textures;
		animation = &data->animationData;
		customTile = data;

		if(tiles[x][y] != PHYSICS_UNOCCUPIED)
		{
			if(!replace)
			{
				delete this;
				return;
			}
			else
			{
				if(layer == LAYER_BACKGROUND)
					delete tilemap_bg[x][y];
				else if(layer == LAYER_FOREGROUND)
					delete tilemap_fg[x][y];
				else if(layer == LAYER_SPECIAL)
					delete tilemap_fg[x][y];
				tiles[x][y] = PHYSICS_UNOCCUPIED;
			}
		}

		tiles[x][y] = type;
		if(layer == LAYER_BACKGROUND)
			tilemap_bg[x][y] = this;
		else if(layer == LAYER_FOREGROUND)
			tilemap_fg[x][y] = this;
		else if(layer == LAYER_FOREGROUND)
			tilemap_fg[x][y] = this;
	}
}

Tile::Tile(int x, int y, CustomTile *data, char type, bool replace, TILEMAP_LAYERS layer)
{
	if(x >= (int)tilemap_bg.size() || y >= (int)tilemap_bg[0].size())
	{
		PrintLog(LOG_IMPORTANT, "Attempted to place a tile outside of level boundaries: %d %d", x, y);
		delete this;
		return;
	}
	else
	{
		this->type = type;
		this->tex_x = data->x_offset;
		this->tex_y = data->y_offset;
		this->x = x;
		this->y = y;
		src_tex = surface_level_textures;
		animation = &data->animationData;

		customTile = data;

		if(tiles[x][y] != PHYSICS_UNOCCUPIED)
		{
			if(!replace)
			{
				delete this;
				return;
			}
			else
			{
				if(layer == LAYER_BACKGROUND)
					delete tilemap_bg[x][y];
				else if(layer == LAYER_FOREGROUND)
					delete tilemap_fg[x][y];
				else if(layer == LAYER_SPECIAL)
					delete tilemap_fg[x][y];
				tiles[x][y] = PHYSICS_UNOCCUPIED;
			}
		}

		tiles[x][y] = type;
		if(layer == LAYER_BACKGROUND)
			tilemap_bg[x][y] = this;
		else if(layer == LAYER_FOREGROUND)
			tilemap_fg[x][y] = this;
		else if(layer == LAYER_SPECIAL)
			tilemap_fg[x][y] = this;
	}
}


int Tile::GetID()
{
	return (this->tex_y / TILESIZE) * 11 + this->tex_x / TILESIZE;
}

bool Tile::HasAnimation()
{
	return !!(int)this->animation->sequence.size();
}

void TilesCleanup()
{
	tilemap_bg.~vector();
	tilemap_fg.~vector();
	tileset.~vector();
}

void UnblitTile(Tile *tile)
{
	if(surface_fg)
	{
		SDL_Rect rect = { tile->x * TILESIZE, tile->y * TILESIZE, TILESIZE, TILESIZE };
		SDL_FillRect(surface_fg, &rect, 0x00000000);
	}
}

Tile::~Tile()
{
	if(tilemap_fg[x][y] == this)
	{
		if(tilemap_bg[x][y])
			tiles[x][y] = tilemap_bg[x][y]->type;
		else tiles[x][y] = PHYSICS_AIR;
		UnblitTile(this);
		tilemap_fg[x][y] = nullptr;
	}
	if(tilemap_bg[x][y] == this)
		tilemap_bg[x][y] = nullptr;
}

PHYSICS_TYPES GetTileTypeAtTiledPos(int x, int y)
{
	return GetTileTypeAtTiledPos({ x, y });
}

PHYSICS_TYPES GetTileTypeAtTiledPos(SDL_Point at)
{
	if(at.x < 0 || at.x >= level->width_in_tiles || at.y < 0 || at.y >= level->height_in_tiles)
		return PHYSICS_TYPES::PHYSICS_OB;
	return static_cast<PHYSICS_TYPES>(tiles[at.x][at.y]);
}

PHYSICS_TYPES GetTileTypeAtPos(int x, int y)
{
	return GetTileTypeAtTiledPos({ x / TILESIZE, y / TILESIZE });
}

PHYSICS_TYPES GetTileTypeAtPos(SDL_Point at)
{
	return GetTileTypeAtPos(at.x, at.y);
}

bool IsSolid(PHYSICS_TYPES type)
{
	return type == PHYSICS_BLOCK || type == PHYSICS_ICEBLOCK || type == PHYSICS_ICE;
}

