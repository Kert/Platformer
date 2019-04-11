#ifndef _level_h_
#define _level_h_ 

#include <SDL.h>
#include <string>
#include <vector>

#define TILESIZE 16

class Level
{
	public:
		std::string fileName;
		std::string musicFileName;
		bool loaded;
		SDL_Point playerSpawn = { 200, 70 };
		SDL_Point exit;
		SDL_Color bgColor;
		int width_in_tiles;
		int height_in_tiles;
		int width_in_pix;
		int height_in_pix;
		std::vector<SDL_Rect> CameraBounds;
		std::vector<SDL_Rect> deathZones;
		std::string tileset_filepath;

	public:
		Level();
		Level(std::string fileName);
		void Init();
		void Cleanup();
		void Reload();
		void UnloadEntities();
		~Level();
		void LoadLevelFromFile(std::string filename);
		void LoadEnemies();
		void LoadEntities();
		void LoadPlayer();
		void LoadNonRandomElements();
		void MakeDoorWithButtons(int x, int y);
};

void LevelCleanup();

#endif
