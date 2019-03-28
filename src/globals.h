#ifndef _globals_h_
#define _globals_h_ 
#include <vector>

enum LOG_LEVELS
{
	LOG_IMPORTANT,
	LOG_INFO,
	LOG_DEBUG,
	LOG_SUPERDEBUG
};

// set this to the highest log level you want to see, or -1 for no log. Comment it out to disable SDL_Log entirely
#define LOGDETAIL LOG_DEBUG

#ifdef _RELEASE
#undef LOGDETAIL
#endif

#ifndef LOGDETAIL
#define SDL_Log(_1, ...)
#define LOGDETAIL -1
#endif

enum GAMESTATES
{
	STATE_GAME,
	STATE_MENU,
	STATE_OPTIONS,
	STATE_PAUSED,
	STATE_TRANSITION
};

enum FADING_STATES
{
	FADING_STATE_NONE,
	FADING_STATE_IN,
	FADING_STATE_OUT,
	FADING_STATE_BLACKNBACK
};

enum PHYSICS_TYPES
{
	PHYSICS_UNOCCUPIED,
	PHYSICS_AIR,
	PHYSICS_BLOCK,
	PHYSICS_HOOK,
	PHYSICS_LADDER,
	PHYSICS_LADDER_TOP,
	PHYSICS_PLATFORM, // one-way
	PHYSICS_EXITBLOCK, // triggers level exiting
	PHYSICS_RAIN,
	PHYSICS_ICEBLOCK,
	PHYSICS_ICE,
	PHYSICS_WATER,
	PHYSICS_WATERTOP,
	PHYSICS_HOOK_PLATFORM,
	PHYSICS_OB // out of bounds
};

enum EFFECT_TYPES
{
	EFFECT_NONE,
	EFFECT_MGUN_HIT,
	EFFECT_ROCKETL_HIT,
	EFFECT_ZAP
};

enum WEAPONS
{
	WEAPON_FIREBALL,
	WEAPON_LIGHTNING,
	WEAPON_ROCKETL,
	WEAPON_FLAME,
	WEAPON_GRENADE,
	WEAPON_BOMBDROP,
	WEAPON_GROUNDSHOCKWAVE,
	NUMWEAPONS
};

#define MAX_LIVES 5

enum CREATURE_STATES
{
	ONGROUND,
	HANGING,
	INAIR,
	SLIDING,
	JUMPING,
	ONLADDER,
	DUCKING
};

enum DIRECTIONS
{
	DIRECTION_LEFT,
	DIRECTION_RIGHT
};

enum STATUSES
{
	STATUS_NORMAL,
	STATUS_INVULN,
	STATUS_STUN,
	STATUS_DYING
};

enum SPAWN_TYPES
{
	SPAWN_PICKUP,
	SPAWN_DOOR,
	SPAWN_PLATFORM,
	SPAWN_LAVA_FLOOR
};

enum PICKUP_TYPES
{
	PICKUP_NOTHING,
	PICKUP_AMMO,
	PICKUP_HEALTH,
	PICKUP_LIGHTNING,
	PICKUP_FIREBALL,
	PICKUP_GRENADE
};

struct TileFrame
{
	int id;
	int duration;
};

struct TileAnimationData
{
	std::vector<TileFrame> sequence;
	unsigned __int32 timer;
	int currentFrame;
};

struct CustomTile
{
	int type;
	int x_offset;
	int y_offset;
	int animated_x_offset;
	int animated_y_offset;
	TileAnimationData animationData;
};

struct QueuedEntity
{
	SPAWN_TYPES entityType;
	int x;
	int y;
	int data;
	int data2;
};

enum ANIM_LOOP_TYPES
{
	LOOP_NONE, // Play animation once
	LOOP_NORMAL,
	LOOP_PINGPONG
};

enum MENUS
{
	MENU_MAIN,
	MENU_OPTIONS,
	MENU_VIDEO_OPTIONS,
	MENU_PAUSE,
	MENU_SELECTION_LIVES,
	MENU_SELECTION_DISPLAY,
	MENU_SELECTION_DISPLAY_MODE,
	MENU_SELECTION_FULLSCREEN,
	MENU_PLAYER_FAILED,
	MENU_PLAYER_FAILED_NO_ESCAPE,
	MENU_BINDS,
	MENU_BIND // should be a transition?
	// otherwise MENU_BINDS must be the last one apart from this
};

enum GAME_OVER_REASONS
{
	GAME_OVER_REASON_WIN,
	GAME_OVER_REASON_DIED,
	GAME_OVER_REASON_TIME
};

enum PLAYER_BODY_PARTS
{
	PLAYER_BODY_TRANSPARENT,
	PLAYER_BODY_OUTLINE,
	PLAYER_BODY_SPECIAL,
	PLAYER_BODY_TAIL,
	PLAYER_BODY_LEGS,
	PLAYER_BODY_MAIN,
	PLAYER_BODY_EYES
};

enum TEXT_ALIGN
{
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
};

#endif
