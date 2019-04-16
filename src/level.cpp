#include "level.h"
#include <SDL_image.h>
#include <vector>
#include "entities.h"
#include "globals.h"
#include "gamelogic.h"
#include "graphics.h"
#include "tiles.h"
#include "tinyxml.h"
#include "utils.h"

Lava_Floor *lava = nullptr;

extern std::vector<CustomTile> tileset;

struct EnemyLoadData
{
	int x;
	int y;
	std::string type;
	std::string AItype;
	int distanceToReachX;
	int distanceToReachY;
	std::string facing;
};

struct PickupLoadData
{
	int x;
	int y;
	PICKUP_TYPES type;
};

struct PlatformLoadData
{
	int x;
	int y;
	std::string type;
	int pathID;
	double speed;
};

std::vector<QueuedEntity> entitySpawns;
std::vector<EnemyLoadData> levelEnemies;
std::vector<PickupLoadData> levelPickups;
std::vector<PlatformLoadData> levelPlatforms;

extern std::vector<std::vector<std::vector<Tile*>>> tileLayers;

std::vector<std::vector<int>> tiles;

Level::Level()
{
	fileName = "test.tmx";
	Init();
}

Level::Level(std::string fileName)
{
	this->fileName = fileName;
	Init();
}

void Level::Init()
{
	loaded = false;

	LoadLevelFromFile(fileName);

	LoadNonRandomElements();
	LoadEnemies();
	LoadEntities();

	width_in_pix = width_in_tiles * TILESIZE;
	height_in_pix = height_in_tiles * TILESIZE;

	// create a deathzone below level
	deathZones.push_back({ 0, height_in_pix + 50, width_in_pix, 50 });

	loaded = true;
}

void Level::LoadLevelFromFile(std::string filename)
{
	filename = "assets/levels/" + filename;
	TiXmlDocument doc(filename.c_str());
	bool loadOkay = doc.LoadFile();

	TiXmlNode* node = doc.FirstChild("map");

	TiXmlElement* mapProperties = node->ToElement();
	
	TiXmlNode* props = mapProperties->FirstChild();
	for(TiXmlElement* prop = props->FirstChildElement(); prop != NULL; prop = prop->NextSiblingElement())
	{
		const char *name = prop->Attribute("name");
		if(name)
		{
			std::string	name_string = name;
			if(name_string == "music")
			{
				const char* tmp = prop->Attribute("value");
				if(tmp)
					musicFileName = tmp;
				else
					musicFileName = "1.ogg";
			}
		}		
	}

	std::string colorstr = mapProperties->Attribute("backgroundcolor");
	sscanf(colorstr.c_str(), "#%02hhx%02hhx%02hhx%02hhx", &this->bgColor.r, &this->bgColor.g, &this->bgColor.b, &this->bgColor.a);

	this->width_in_tiles = SDL_atoi(mapProperties->Attribute("width"));
	this->height_in_tiles = SDL_atoi(mapProperties->Attribute("height"));
	tiles = std::vector<std::vector<int>>(this->width_in_tiles, std::vector<int>(this->height_in_tiles));
	
	TiXmlNode* tileset_data = node->FirstChild("tileset");
	std::string tileset_name;
	for(TiXmlElement* img = tileset_data->FirstChildElement("image"); img != NULL; img = img->NextSiblingElement("image"))
	{
		tileset_filepath = img->Attribute("source");
		std::vector<std::string> tokens;
		tokenize(tileset_filepath, tokens, "/");
		tileset_name = tokens.back();
		tileset_filepath = "assets/textures/" + tileset_name;
		Graphics::LoadLevelTexturesFromFile(tileset_filepath);
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
		tileLayers.push_back(std::vector< std::vector<Tile*>>(this->width_in_tiles, std::vector<Tile*>(this->height_in_tiles)));
		
		int tileColumn = 0;
		int tileRow = 0;
		TiXmlElement* curData = curLayer->FirstChildElement("data");
		for(TiXmlElement* curTile = curData->FirstChildElement("tile"); curTile != NULL; curTile = curTile->NextSiblingElement("tile"))
		{
			const char* gid = curTile->Attribute("gid");
			int type;
			if (gid == nullptr)
				type = 0;
			else
				type = SDL_atoi(gid);
			if(type)
			{
				new Tile(tileColumn, tileRow, layerNum, &tileset[type - 1], true);
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
		const char* typeAttr = obj->Attribute("type");
		if(!typeAttr)
		{
			PrintLog(LOG_IMPORTANT, "Object does not have any type");
			continue;
		}
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
				levelEnemies.push_back(EnemyLoadData{ x, y, name, AItype, distanceToReachX, distanceToReachY, facing });
			}
		}
		if(type == "platform")
		{
			int x = SDL_atoi(obj->Attribute("x"));
			int y = SDL_atoi(obj->Attribute("y"));
			
			const char *type = obj->Attribute("name");
			if(!type)
			{
				PrintLog(LOG_IMPORTANT, "Platform type is null. Setting default");
				type = "alien_platform";
			}

			int pathID = -1;
			double speed = 50;
			TiXmlElement* prop = obj->FirstChildElement();
			if(prop)
			{
				for(TiXmlElement* curProp = prop->FirstChildElement("property"); curProp != NULL; curProp = curProp->NextSiblingElement("property"))
				{
					std::string propName = curProp->Attribute("name");
					if(propName == "pathID")
						pathID = atoi(curProp->Attribute("value"));
					else if(propName == "speed")
						speed = atof(curProp->Attribute("value"));
				}
			}
			PlatformLoadData data;
			data.x = x;
			data.y = y;
			data.type = type;
			data.pathID = pathID;
			data.speed = speed;
			levelPlatforms.push_back(data);
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
			PICKUP_TYPES type;
			if(name == "health")
				type = PICKUP_HEALTH;
			else if(name == "lightning")
				type = PICKUP_LIGHTNING;
			else if(name == "fireball")
				type = PICKUP_FIREBALL;
			else
			{
				PrintLog(LOG_IMPORTANT, "Invalid Pickup type %s", name.c_str());
				continue;
			}
			PickupLoadData data;
			data.x = x;
			data.y = y;
			data.type = type;
			levelPickups.push_back(data);
		}
		if(type == "path")
		{
			SDL_Point initialPoint;
			initialPoint.x = atoi(obj->Attribute("x"));
			initialPoint.y = atoi(obj->Attribute("y"));

			int pathID = atoi(obj->Attribute("id"));

			TiXmlElement* pathObj = obj->FirstChildElement();
			if(!pathObj)
				continue;

			Path path;
			std::string pathType = pathObj->Value();
			if(pathType != "polyline" && pathType != "polygon")
				continue;
			
			std::string pointsString = pathObj->Attribute("points");
			std::vector<std::string> tokens;
			tokenize(pointsString, tokens, " ");
			for(auto token : tokens)
			{
				SDL_Point point;
				sscanf(token.c_str(), "%d,%d", &point.x, &point.y);
				point.x += initialPoint.x;
				point.y += initialPoint.y;
				path.points.push_back(point);
			}
			if(pathType == "polygon")
				path.loopable = true;

			paths[pathID] = path;
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
	tileset.clear();
	UnloadEntities();
	entitySpawns.clear();
	levelEnemies.clear();
	levelPickups.clear();
	levelPlatforms.clear();
	CameraBounds.clear();
	deathZones.clear();
	paths.clear();
	Game::Reset();
}

Level::~Level()
{
	Cleanup();
}

void Level::UnloadEntities()
{
	DeleteAllEntities();
	Game::RemovePlayer();
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
		else if(e.AItype == "GroundShockwaver")
			c->SetAI<AI_GroundShockwaver>();
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
	for(auto p : levelPickups)
	{
		Pickup *pi = new Pickup(p.type);
		pi->SetPos(p.x, p.y);
	}

	for(auto p : levelPlatforms)
	{
		new Platform(p.x, p.y, p.type, p.pathID, p.speed);
	}

	for(auto e : entitySpawns)
	{
		if(e.entityType == SPAWN_DOOR)
		{
			if(e.data < 2)
			{
				Door *d = new Door(e.x, e.y, e.data > 0);
			}
		}
		/*else if(e.entityType == SPAWN_LAVA_FLOOR)
		{
			lava = new Lava_Floor(e.x, e.y);
		}*/
	}
}

void Level::LoadPlayer()
{
	Player* player = Game::CreatePlayer();
	player->SetPos(playerSpawn.x, playerSpawn.y);
	player->GiveWeapon(WEAPON_FIREBALL);
}

void Level::LoadNonRandomElements()
{
	// Loading player here because game camera needs its position to work with
	LoadPlayer();
	Graphics::CreateCamera();
}

void Level::MakeDoorWithButtons(int x, int y)
{
	new Door(x, y, true);
}

Path* Level::GetPath(int id)
{
	if(paths.find(id) != paths.end())
		return &paths[id];
	return nullptr;
}