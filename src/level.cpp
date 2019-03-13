#include "level.h"
#include <SDL_image.h>
#include <vector>
#include "camera.h"
#include "entities.h"
#include "globals.h"
#include "gamelogic.h"
#include "graphics.h"
#include "tiles.h"
#include "tinyxml.h"
#include "utils.h"

extern Player *player;
extern Camera* camera;

Lava_Floor *lava = nullptr;

extern int timeLimit;
Level *level = nullptr;

extern std::vector<CustomTile> tileset;

extern int GAME_SCENE_WIDTH;
extern int GAME_SCENE_HEIGHT;

struct EnemyData
{
	int x;
	int y;
	std::string type;
	std::string AItype;
	int distanceToReachX;
	int distanceToReachY;
	std::string facing;
};

std::vector<QueuedEntity> entitySpawns;
std::vector<EnemyData> levelEnemies;

extern std::vector< std::vector<Tile*> > tilemap_bg;
extern std::vector< std::vector<Tile*> > tilemap_fg;

std::vector<std::vector<int>> tiles;
extern int map_width;
extern int map_height;
extern SDL_Surface *surface_level_textures;

Level::Level()
{
	Init();
}

void Level::Init()
{
	loaded = false;

	LoadLevelFromFile("assets/levels/test.tmx");
	//LoadLevelFromFile("assets/levels/volcano_surroundings.tmx");
	LoadNonRandomElements();
	LoadEnemies();
	LoadEntities();

	map_width = width_in_pix = width_in_tiles * TILESIZE;
	map_height = height_in_pix = height_in_tiles * TILESIZE;

	// create a deathzone below level
	deathZones.push_back({ 0, height_in_pix + 50, width_in_pix, 50 });

	SetupLevelGraphics(map_width, map_height);

	loaded = true;
}

void Level::LoadLevelFromFile(std::string filename)
{
	TiXmlDocument doc(filename.c_str());
	bool loadOkay = doc.LoadFile();

	TiXmlNode* node = doc.FirstChild("map");

	TiXmlElement* mapProperties = node->ToElement();

	std::string colorstr = mapProperties->Attribute("backgroundcolor");
	sscanf(colorstr.c_str(), "#%02hhx%02hhx%02hhx%02hhx", &this->bgColor.r, &this->bgColor.g, &this->bgColor.b, &this->bgColor.a);

	this->width_in_tiles = SDL_atoi(mapProperties->Attribute("width"));
	this->height_in_tiles = SDL_atoi(mapProperties->Attribute("height"));
	tiles = std::vector<std::vector<int>>(this->width_in_tiles, std::vector<int>(this->height_in_tiles));
	tilemap_bg = std::vector< std::vector<Tile*>>(this->width_in_tiles, std::vector<Tile*>(this->height_in_tiles));
	tilemap_fg = std::vector< std::vector<Tile*>>(this->width_in_tiles, std::vector<Tile*>(this->height_in_tiles));

	TiXmlNode* tileset_data = node->FirstChild("tileset");
	std::string tileset_name;
	for(TiXmlElement* img = tileset_data->FirstChildElement("image"); img != NULL; img = img->NextSiblingElement("image"))
	{
		tileset_filepath = img->Attribute("source");
		std::vector<std::string> tokens;
		tokenize(tileset_filepath, tokens, "/");
		tileset_name = tokens.back();
		tileset_filepath = "assets/textures/" + tileset_name;
		surface_level_textures = IMG_Load(tileset_filepath.c_str());
		LoadTileSet();
		break; // not supporting multiple tilesets currently
	}

	// Loading tileset types and animations
	for(TiXmlElement* tile = tileset_data->FirstChildElement("tile"); tile != NULL; tile = tile->NextSiblingElement("tile"))
	{
		int type = PHYSICS_AIR;
		int id = SDL_atoi(tile->Attribute("id"));
		const char *tmp = tile->Attribute("type");
		if(tmp != nullptr)
		{
			std::string	type_string = tmp;
			if(type_string == "block" || type_string == std::to_string(PHYSICS_BLOCK))
				type = PHYSICS_BLOCK;
			else if(type_string == "hook" || type_string == std::to_string(PHYSICS_HOOK))
				type = PHYSICS_HOOK;
			else if(type_string == "hook_platform" || type_string == std::to_string(PHYSICS_HOOK_PLATFORM))
				type = PHYSICS_HOOK_PLATFORM;
			else if(type_string == "platform" || type_string == std::to_string(PHYSICS_PLATFORM))
				type = PHYSICS_PLATFORM;
			else if(type_string == "exit" || type_string == std::to_string(PHYSICS_EXITBLOCK))
				type = PHYSICS_EXITBLOCK;
			else if(type_string == "rain" || type_string == std::to_string(PHYSICS_RAIN))
				type = PHYSICS_RAIN;
			else if(type_string == "iceblock" || type_string == std::to_string(PHYSICS_ICEBLOCK))
				type = PHYSICS_ICEBLOCK;
			else if(type_string == "ice" || type_string == std::to_string(PHYSICS_ICE))
				type = PHYSICS_ICE;
			else if(type_string == "water" || type_string == std::to_string(PHYSICS_WATER))
				type = PHYSICS_WATER;
			else if(type_string == "watertop" || type_string == std::to_string(PHYSICS_WATERTOP))
				type = PHYSICS_WATERTOP;
			tileset[id].type = type;
		}
		TiXmlElement* anim = tile->FirstChildElement("animation");
		if(anim == NULL)
			continue;
		for(TiXmlElement* frame = anim->FirstChildElement("frame"); frame != NULL; frame = frame->NextSiblingElement("frame"))
		{
			int tileid = SDL_atoi(frame->Attribute("tileid"));
			int duration = SDL_atoi(frame->Attribute("duration"));
			tileset[id].animationData.sequence.push_back(TileFrame{ tileid, duration });
		}
	}

	int layerNum = 0;
	for(TiXmlElement* curLayer = node->FirstChildElement("layer"); curLayer != NULL; curLayer = curLayer->NextSiblingElement("layer"))
	{
		int tileColumn = 0;
		int tileRow = 0;
		TiXmlElement* curData = curLayer->FirstChildElement("data");
		for(TiXmlElement* curTile = curData->FirstChildElement("tile"); curTile != NULL; curTile = curTile->NextSiblingElement("tile"))
		{
			int type = SDL_atoi(curTile->Attribute("gid"));
			if(type)
			{
				TILEMAP_LAYERS l;
				if(!layerNum)
					l = LAYER_BACKGROUND;
				else if(layerNum == LAYER_FOREGROUND)
					l = LAYER_FOREGROUND;
				else if(layerNum = LAYER_SPECIAL)
					l = LAYER_FOREGROUND;
				TileCoord t = { tileColumn, tileRow, l };
				new Tile(tileColumn, tileRow, &tileset[type - 1], true, l);
			}
			tileColumn++;
			if(tileColumn >= this->width_in_tiles)
			{
				tileRow++;
				tileColumn = 0;
			}
		}
		layerNum++;
	}

	TiXmlNode* objects = node->FirstChild("objectgroup");
	for(TiXmlElement* obj = objects->FirstChildElement("object"); obj != NULL; obj = obj->NextSiblingElement("object"))
	{
		std::string type = obj->Attribute("type");
		if(type == "cam")
		{
			SDL_Rect r;
			r.h = SDL_atoi(obj->Attribute("height"));
			r.w = SDL_atoi(obj->Attribute("width"));
			r.x = SDL_atoi(obj->Attribute("x"));
			r.y = SDL_atoi(obj->Attribute("y"));
			CameraBounds.push_back(r);
		}
		if(type == "deathzone")
		{
			SDL_Rect r;
			r.h = SDL_atoi(obj->Attribute("height"));
			r.w = SDL_atoi(obj->Attribute("width"));
			r.x = SDL_atoi(obj->Attribute("x"));
			r.y = SDL_atoi(obj->Attribute("y"));
			deathZones.push_back(r);
		}
		if(type == "spawn")
		{
			std::string name = obj->Attribute("name");
			if(name == "player")
			{
				SDL_Point p;
				p.x = SDL_atoi(obj->Attribute("x"));
				p.y = SDL_atoi(obj->Attribute("y"));
				this->playerSpawn = p;
			}
			else
			{
				int x = SDL_atoi(obj->Attribute("x"));
				int y = SDL_atoi(obj->Attribute("y"));

				TiXmlElement* prop = obj->FirstChildElement();
				if(!prop)
					continue;

				std::string AItype;
				int distanceToReachX = 200;
				int distanceToReachY = 200;
				std::string facing = "left";
				for(TiXmlElement* curProp = prop->FirstChildElement("property"); curProp != NULL; curProp = curProp->NextSiblingElement("property"))
				{
					std::string propName = curProp->Attribute("name");
					if(propName == "ai")
						AItype = curProp->Attribute("value");
					else if(propName == "distanceToReachX")
						distanceToReachX = SDL_atoi(curProp->Attribute("value"));
					else if(propName == "distanceToReachY")
						distanceToReachY = SDL_atoi(curProp->Attribute("value"));
					else if(propName == "facing")
						facing = curProp->Attribute("value");
				}
				levelEnemies.push_back(EnemyData{ x, y, name, AItype, distanceToReachX, distanceToReachY, facing });
			}
		}
		if(type == "platform")
		{
			QueuedEntity q;
			int x = SDL_atoi(obj->Attribute("x"));
			int y = SDL_atoi(obj->Attribute("y"));
			int h = SDL_atoi(obj->Attribute("height"));
			int w = SDL_atoi(obj->Attribute("width"));
			if(h > w)
				q = { SPAWN_PLATFORM, x, y + TILESIZE, x, y + h };
			else
				q = { SPAWN_PLATFORM, x, y, x + w - TILESIZE * 2, y };
			entitySpawns.push_back(q);
		}
		if(type == "lava_floor")
		{
			QueuedEntity q;
			int x = SDL_atoi(obj->Attribute("x"));
			int y = SDL_atoi(obj->Attribute("y"));
			q = { SPAWN_LAVA_FLOOR, x, y - TILESIZE, NULL, NULL };
			entitySpawns.push_back(q);
		}
		if(type == "pickup")
		{
			std::string name = obj->Attribute("name");
			int x = SDL_atoi(obj->Attribute("x"));
			int y = SDL_atoi(obj->Attribute("y")) + TILESIZE;
			Pickup *pi;
			if(name == "health")
				pi = new Pickup(PICKUP_HEALTH);
			else if(name == "lightning")
				pi = new Pickup(PICKUP_LIGHTNING);
			else if(name == "fireball")
				pi = new Pickup(PICKUP_FIREBALL);
			else
			{
				PrintLog(LOG_IMPORTANT, "Invalid Pickup type %s", name.c_str());
				continue;
			}
			pi->SetPos(x, y);
		}
	}
}

void Level::Reload()
{
	Cleanup();
	Init();
}

void Level::Cleanup()
{
	DeleteAllTiles();
	ResetLevelGraphics();
	UnloadEntities();
	entitySpawns.clear();
	levelEnemies.clear();
	CameraBounds.clear();
	deathZones.clear();
	ResetLogic();
}

Level::~Level()
{
	Cleanup();
}

void Level::UnloadEntities()
{
	DeleteAllEntities();
	delete camera;
	camera = nullptr;
	delete player;
	player = nullptr;
}

void Level::LoadEnemies()
{
	for(auto &e : levelEnemies)
	{
		Creature *c;
		c = new Creature(e.type);
		if(c == nullptr)
		{
			PrintLog(LOG_IMPORTANT, "Invalid Creature type %s", e.type);
			continue;
		}
		c->SetPos(e.x, e.y);
		if(e.AItype == "Chaser")
			c->SetAI<AI_Chaser>();
		else if(e.AItype == "ChaserJumper")
			c->SetAI<AI_ChaserJumper>();
		else if(e.AItype == "Wanderer")
			c->SetAI<AI_Wanderer>();
		else if(e.AItype == "HomingMissile")
			c->SetAI<AI_HomingMissile>();
		else if(e.AItype == "Hypno")
			c->SetAI<AI_Hypno>();
		else if(e.AItype == "Liner")
			c->SetAI<AI_Liner>();
		else if(e.AItype == "Sentinel")
			c->SetAI<AI_Sentinel>();
		else if(e.AItype == "Anvil")
			c->SetAI<AI_Anvil>();
		else if(e.AItype == "Jumpingfire")
			c->SetAI<AI_Jumpingfire>();
		else
		{
			PrintLog(LOG_IMPORTANT, "Invalid AI type %s", e.AItype.c_str());
			continue;
		}
		c->AI->SetDistanceToReachX(e.distanceToReachX * TILESIZE);
		c->AI->SetDistanceToReachY(e.distanceToReachY * TILESIZE);
		if(e.facing == "left")
			c->direction = DIRECTION_LEFT;
		else if(e.facing == "right")
			c->direction = DIRECTION_RIGHT;
		else
		{
			PrintLog(LOG_IMPORTANT, "Invalid facing direction %s", e.facing.c_str());
			continue;
		}
	}
}

void Level::LoadEntities()
{
	for(auto e : entitySpawns)
	{
		if(e.entityType == SPAWN_DOOR)
		{
			if(e.data < 2)
			{
				Door *d = new Door(e.x, e.y, e.data > 0);
			}
		}
		else if(e.entityType == SPAWN_PICKUP)
		{
			Pickup *p = new Pickup((PICKUP_TYPES)e.data);
			p->SetPos(e.x, e.y);
		}
		else if(e.entityType == SPAWN_PLATFORM)
		{
			new Platform(e.x, e.y, e.data, e.data2);
		}
		/*else if(e.entityType == SPAWN_LAVA_FLOOR)
		{
			lava = new Lava_Floor(e.x, e.y);
		}*/
	}
}

void Level::LoadPlayer()
{
	player = new Player();
	player->SetPos(playerSpawn.x, playerSpawn.y);
	player->GiveWeapon(WEAPON_FIREBALL);
}

void Level::LoadNonRandomElements()
{
	// Loading player here because game camera needs its position to work with
	LoadPlayer();

	camera = new Camera(0, 0, GAME_SCENE_WIDTH, GAME_SCENE_HEIGHT);
	camera->Attach(*player);
}

void Level::MakeDoorWithButtons(int x, int y)
{
	new Door(x, y, true);
}

void LevelCleanup()
{
	if(level != nullptr)
		delete level;
}