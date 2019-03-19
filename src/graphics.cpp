#include "graphics.h"
#include <SDL_image.h>
#include "animation.h"
#include "camera.h"
#include "entities.h"
#include "interface.h"
#include "level.h"
#include "state.h"
#include "tiles.h"
#include "menu.h"
#include "transition.h"
#include "utils.h"

extern bool IsDebugMode;
bool RENDER_ONLY_OBSERVABLE = true;

Camera* camera;
extern Player *player;

bool graphicsLoaded = false;
extern bool menuLoaded;
extern bool gameLoaded;

extern int TransitionID;
extern MENUS CurrentMenu;
extern std::vector<Menu*> menus;
extern int BindingKey;
extern GAME_OVER_REASONS gameOverReason;

std::map<int, std::vector<SDL_DisplayMode>> displayModes;

SDL_DisplayMode displayMode;
int displayIndex;
int fullscreenMode;

Timer timer100{ 100 }, timerRain{ 200 };

std::vector<Timer*> TimersGraphics{ &timer100, &timerRain };

extern int playerLives;
extern int currentLives;

extern std::vector<Bullet*> bullets;
extern std::vector<Effect*> effects;
extern std::vector<Creature*> creatures;
extern std::vector<Pickup*> pickups;
extern std::vector<Machinery*> machinery;
extern std::vector<Lightning*> lightnings;

int map_width;
int map_height;

static int const MAX_TILES_VERTICALLY = 18;

int WINDOW_WIDTH;
int WINDOW_HEIGHT;

// SHOULD CALCULATE THIS
int RENDER_SCALE = 1;

// in tiles
int GAME_SCENE_WIDTH;
int GAME_SCENE_HEIGHT;

SDL_Renderer *renderer = NULL;
SDL_Surface *player_surf = NULL;
SDL_Texture *player_texture = NULL;
SDL_Texture *level_bgLayer_tex = NULL;
SDL_Texture *level_fgLayer_tex = NULL;
SDL_Surface *surface_bg = NULL;
SDL_Surface *surface_fg = NULL;
SDL_Surface *surface_level_textures = NULL;
SDL_Surface *lightningSegment = NULL;

SDL_Surface *pov_surface = NULL;
SDL_Surface *fading_surface = NULL;

SDL_Window *win = NULL;

TTF_Font *debug_font = NULL;
TTF_Font *menu_font = NULL;
TTF_Font *game_font = NULL;
TTF_Font *minor_font = NULL;
SDL_Color debug_color = { 50, 180, 0 };
SDL_Color pause_color = { 40, 100, 0 };
SDL_Color menu_color = { 115, 77, 0 };
SDL_Color selected_color = { 100, 100, 255 };
SDL_Surface *debug_message = NULL;

extern std::vector< std::vector<Tile*> > tilemap_bg;
extern std::vector< std::vector<Tile*> > tilemap_fg;

SDL_Window *mapwin = NULL;
SDL_Renderer *maprenderer = NULL;
SDL_Texture *mapoverview_texture = NULL;

extern int SelectedItem;
extern int timeLimit;
extern int FadingVal;

extern Level *level;

int GetDisplayModes();
int SetDisplayMode(SDL_DisplayMode mode);

TextureManager textureManager;

bool TextureManager::IsLoaded(std::string filename)
{
	return textures.find(filename) != textures.end();
}

void TextureManager::LoadTexture(std::string filename)
{
	if(IsLoaded(filename))
	{
		PrintLog(LOG_DEBUG, "Texture %s is already loaded", filename);
		return;
	}
	SDL_Texture *tex = IMG_LoadTexture(renderer, filename.c_str());
	textures[filename] = tex;
}

void TextureManager::UnloadTexture(std::string filename)
{
	if(!IsLoaded(filename))
	{
		PrintLog(LOG_DEBUG, "Texture %s is not loaded", filename.c_str());
		return;
	}
	SDL_DestroyTexture(textures[filename]);
	textures.erase(textures.find(filename));
}

SDL_Texture** TextureManager::GetTexture(std::string filename)
{
	if(!IsLoaded(filename))
	{
		PrintLog(LOG_DEBUG, "Texture %s is not loaded", filename.c_str());
		PrintLog(LOG_DEBUG, "Loading %s ", filename.c_str());
		LoadTexture(filename);
	}
	return &textures[filename];
}

void TextureManager::Clear()
{
	for(auto i : textures)
		SDL_DestroyTexture(i.second);
	textures.clear();
}

void CreateMapWindow()
{
	mapwin = SDL_CreateWindow("map overview", 1000, 100, OVERVIEW_WIDTH, OVERVIEW_HEIGHT, NULL);
	maprenderer = SDL_CreateRenderer(mapwin, -1, SDL_RENDERER_ACCELERATED);
}

void InitPlayerTexture()
{
	if(player_texture)
		SDL_DestroyTexture(player_texture);
	player_texture = SDL_CreateTextureFromSurface(renderer, player_surf);
}

void ChangePlayerColor(PLAYER_BODY_PARTS bodyPart, SDL_Color color)
{
	player_surf->format->palette->colors[bodyPart] = color;
	InitPlayerTexture();
}

int GraphicsSetup()
{
	if(graphicsLoaded) return 0;

	// Initialize SDL_TTF for font rendering
	if(TTF_Init() == -1)
		return 0;

	debug_font = TTF_OpenFont("assets/misc/verdana.ttf", 12);
	menu_font = TTF_OpenFont("assets/misc/verdana.ttf", 28);
	game_font = TTF_OpenFont("assets/misc/InfiniumGuardian.ttf", 28);
	minor_font = TTF_OpenFont("assets/misc/verdana.ttf", 22);

	// create the window and renderer
	// note that the renderer is accelerated
	win = SDL_CreateWindow("Platformer", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

	SDL_Surface *icon = IMG_Load("assets/misc/icon.png");
	SDL_SetWindowIcon(win, icon);
	SDL_FreeSurface(icon);

	GetDisplayModes();

	UpdateDisplayMode();

	// Allows drawing half-transparent rectangles
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	pov_surface = SDL_CreateRGBSurface(0, GAME_SCENE_WIDTH, GAME_SCENE_HEIGHT, 32,
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000);

	InterfaceSetup();

	fading_surface = SDL_CreateRGBSurface(0, 1, 1, 32,
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000);

	player_surf = IMG_Load("assets/sprites/mong.png");

	lightningSegment = IMG_Load("assets/textures/millhilightning.png");
	
	if(IsDebugMode)
		CreateMapWindow();
	graphicsLoaded = true;
	return 1;
}

int GetDisplayModes()
{
	int display_count = 0, display_index = 0, mode_index = 0;
	SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

	if((display_count = SDL_GetNumVideoDisplays()) < 1)
	{
		SDL_Log("SDL_GetNumVideoDisplays returned: %i", display_count);
		return 1;
	}

	for(int j = 0; j < display_count; j++)
	{
		int numDisplayModes = SDL_GetNumDisplayModes(0);
		for(int i = 0; i < numDisplayModes; i++)
		{
			if(SDL_GetDisplayMode(display_index, i, &mode) != 0)
			{
				PrintLog(LOG_IMPORTANT, "SDL_GetDisplayMode failed: %s", SDL_GetError());
				return 1;
			}
			PrintLog(LOG_SUPERDEBUG, "SDL_GetDisplayMode: %i %i %i", SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);
			displayModes[j].push_back(mode);
		}
	}
	return 0;
}

int SetDisplayMode(SDL_DisplayMode mode)
{
	WINDOW_WIDTH = mode.w;
	WINDOW_HEIGHT = mode.h;

	SDL_SetWindowSize(win, WINDOW_WIDTH, WINDOW_HEIGHT);
	SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	
	RENDER_SCALE = static_cast<int>(round((double)WINDOW_HEIGHT / (double)MAX_TILES_VERTICALLY / (double)(TILESIZE)));

	GAME_SCENE_WIDTH = static_cast<int>(ceil((double)WINDOW_WIDTH / RENDER_SCALE));
	GAME_SCENE_HEIGHT = static_cast<int>(ceil((double)WINDOW_HEIGHT / RENDER_SCALE));

	if(pov_surface)
	{
		SDL_FreeSurface(pov_surface);
		pov_surface = SDL_CreateRGBSurface(0, GAME_SCENE_WIDTH, GAME_SCENE_HEIGHT, 32,
			0x00FF0000,
			0x0000FF00,
			0x000000FF,
			0xFF000000);
	}

	return 0;
}

void UpdateDisplayMode()
{
	SDL_WindowFlags flag;
	switch(fullscreenMode)
	{
		case 0:
			flag = (SDL_WindowFlags)0;
			break;
		case 1:
			flag = SDL_WINDOW_FULLSCREEN;
			break;
		case 2:
			flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
			break;
		default:
			flag = (SDL_WindowFlags)0;
			break;
	}

	SDL_SetWindowFullscreen(win, flag);
	if(flag == SDL_WINDOW_FULLSCREEN_DESKTOP)
	{
		SDL_GetWindowDisplayMode(win, &displayMode);
		displayIndex = SDL_GetWindowDisplayIndex(win);
		return;
	}
	
	if(!displayModes.count(displayIndex))
	{
		PrintLog(LOG_IMPORTANT, "Display %i doesn't exist. Defaulting to 0", displayIndex);
		displayIndex = 0;
	}
	for(auto mode : displayModes[displayIndex])
	{
		if(mode.h == displayMode.h &&
			mode.w == displayMode.w &&
			mode.refresh_rate == displayMode.refresh_rate &&
			mode.format == displayMode.format)
		{
			SetDisplayMode(mode);
			// TODO: Check for leaks
			MenusCleanup();
			LoadMenus();
			return;
		}
	}
}

void ResetLevelGraphics()
{
	if(!RENDER_ONLY_OBSERVABLE)
	{
		SDL_DestroyTexture(level_bgLayer_tex);
		SDL_DestroyTexture(level_fgLayer_tex);
		SDL_DestroyTexture(mapoverview_texture);
		level_bgLayer_tex = nullptr;
		level_fgLayer_tex = nullptr;
		mapoverview_texture = nullptr;
		SDL_FreeSurface(surface_bg);
		SDL_FreeSurface(surface_fg);
		surface_bg = nullptr;
		surface_fg = nullptr;
		//SDL_FillRect(surface_bg, NULL, 0x000000);
		//SDL_FillRect(surface_fg, NULL, 0x000000);
	}
}

void DrawFPS(Uint32 dt)
{
	static int oldfps = 0;
	int diff;
	int newfps;
	newfps = (int)(1000 / (dt + 0.01));
	diff = newfps - oldfps;
	if(abs(diff) > 1)
		oldfps = oldfps + (int)(0.5 * diff);
	RenderText(10, 10, "fps: " + std::to_string(newfps), debug_font, debug_color);
}

void BlitTileAt(Tile* tile, int x, int y)
{
	if(tile == NULL) return;
	SDL_Rect rect;
	SDL_Rect rect2;

	rect.x = tile->tex_x;
	rect.y = tile->tex_y;
	rect.w = TILESIZE; rect.h = TILESIZE;

	rect2.x = x * TILESIZE;
	rect2.y = y * TILESIZE;
	rect2.w = TILESIZE; rect2.h = TILESIZE;

	//PrintLog(LOG_SUPERDEBUG, "x= %d y= %d ", rect2.x, rect2.y);

	SDL_BlitScaled(tile->src_tex, &rect, pov_surface, &rect2);
}

void BlitObserveTileAt(Tile* tile, int x, int y)
{
	if(tile == NULL) return;
	SDL_Rect rect;
	SDL_Rect rect2;

	rect.x = tile->tex_x;
	rect.y = tile->tex_y;
	rect.w = TILESIZE; rect.h = TILESIZE;

	rect2.x = x;
	rect2.y = y;
	rect2.w = rect.w; rect2.h = rect.h;

	//PrintLog(LOG_SUPERDEBUG, ("x= %d y= %d ", rect2.x, rect2.y);

	SDL_BlitScaled(tile->src_tex, &rect, pov_surface, &rect2);
}

void BlitObservableTiles()
{
	PrecisionRect prect = camera->GetPRect();
	int x, y;
	x = ConvertToTileCoord(prect.x, false);
	y = ConvertToTileCoord(prect.y, false);
	int w, h;
	w = ConvertToTileCoord(prect.w, true);
	h = ConvertToTileCoord(prect.h, true);

	// range checks
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x >= level->width_in_tiles) x = level->width_in_tiles - 1;
	if(y >= level->height_in_tiles) x = level->height_in_tiles - 1;

	// clear with background color
	SDL_FillRect(pov_surface, NULL, SDL_MapRGBA(pov_surface->format, level->bgColor.r, level->bgColor.g, level->bgColor.b, 255));
	int a = SDL_GetTicks();
	// counters for current tile pos to blit to
	int p, q;
	p = (int)(x * TILESIZE - prect.x);
	for(int i = x; i <= x + w; i++)
	{
		q = (int)(y * TILESIZE - prect.y);
		for(int j = y; j <= y + h; j++)
		{
			if(i >= 0 && j >= 0 && i < level->width_in_tiles && j < level->height_in_tiles)
			{
				BlitObserveTileAt(tilemap_bg[i][j], p, q);
				BlitObserveTileAt(tilemap_fg[i][j], p, q);
				q += TILESIZE;
			}
		}
		p += TILESIZE;
	}
	int b = SDL_GetTicks();
	//PrintLog(LOG_DEBUG, "%d time passed", b - a);
	// transfer pixedata from surface to texture

	SDL_Texture *pov_texture = SDL_CreateTextureFromSurface(renderer, pov_surface);
	SDL_Rect dest;
	dest.x = dest.y = 0;
	dest.w = GAME_SCENE_WIDTH * RENDER_SCALE;
	dest.h = GAME_SCENE_HEIGHT * RENDER_SCALE;
	SDL_RenderCopy(renderer, pov_texture, NULL, &dest);
	SDL_DestroyTexture(pov_texture);
}

void GraphicsUpdate()
{
	for(auto &t : TimersGraphics)
	{
		t->Run();
	}

	UpdateTileAnimations();

	if(RENDER_ONLY_OBSERVABLE)
		BlitObservableTiles();
	else
	{
		// background color
		SDL_SetRenderDrawColor(renderer, level->bgColor.r, level->bgColor.g, level->bgColor.b, 255);
		SDL_RenderFillRect(renderer, NULL);

		// show level map
		SDL_RenderCopy(renderer, level_bgLayer_tex, &camera->GetRect(), NULL);
		void *pixels;
		int pitch;
		SDL_LockTexture(level_fgLayer_tex, NULL, &pixels, &pitch);
		memcpy(pixels, surface_fg->pixels, surface_fg->pitch * surface_fg->h);
		SDL_UnlockTexture(level_fgLayer_tex);
		SDL_RenderCopy(renderer, level_fgLayer_tex, &camera->GetRect(), NULL);
	}

	// Renders everything in entities collections
	for(auto &dy : machinery)
		Render(*dy);
	for(auto &p : pickups)
	{
		UpdateAnimation(*p);
		Render(*p);
	}
	for(auto &d : creatures)
	{
		UpdateAnimation(*d);
		Render(*d);
	}
	for(auto &l : lightnings)
	{
		//UpdateAnimation(*l);
		Render(*l);
	}
	UpdateAnimation(*player);
	Render(*player);
	for(auto &b : bullets)
	{
		UpdateAnimation(*b);
		Render(*b);
	}
	for(auto &e : effects)
	{
		UpdateAnimation(*e);
		Render(*e);
	}

	RenderInterface();

	int healthFrame = ((100 - player->health) / 25);
	ChangeInterfaceFrame(healthFrame, INTERFACE_LIFE);

	if(IsDebugMode) ShowDebugInfo(*player);

	if(IsDebugMode)
	{
		// background color
		SDL_SetRenderDrawColor(maprenderer, 210, 226, 254, 255);
		SDL_RenderFillRect(maprenderer, NULL);

		SDL_Rect r;
		r.w = OVERVIEW_WIDTH;
		r.h = OVERVIEW_HEIGHT;
		r.x = r.y = 0;
		if(OVERVIEW_WIDTH <= OVERVIEW_HEIGHT)
		{
			r.h = level->height_in_pix;
			if(level->width_in_pix / OVERVIEW_WIDTH)
				r.h /= level->width_in_pix / OVERVIEW_WIDTH;
		}
		else
		{
			r.w = level->width_in_pix;
			if(level->height_in_pix / OVERVIEW_HEIGHT)
				r.w /= level->height_in_pix / OVERVIEW_HEIGHT;
		}
		SDL_RenderCopy(maprenderer, mapoverview_texture, NULL, &r);
	}
}

void BlitTile(Tile *tile, TILEMAP_LAYERS layer)
{
	SDL_Rect rect;
	SDL_Rect rect2;
	rect.x = tile->tex_x;
	rect.y = tile->tex_y;
	rect.w = TILESIZE; rect.h = TILESIZE;
	rect2.x = tile->x * TILESIZE;
	rect2.y = tile->y * TILESIZE;
	rect2.w = TILESIZE; rect2.h = TILESIZE;
	//PrintLog(LOG_SUPERDEBUG, ("x= %d y= %d ", rect2.x, rect2.y);
	if(layer == LAYER_BACKGROUND)
		SDL_BlitScaled(tile->src_tex, &rect, surface_bg, &rect2);
	if(layer == LAYER_FOREGROUND)
	{
		if(tile->type != PHYSICS_RAIN)
			SDL_BlitScaled(tile->src_tex, &rect, surface_fg, &rect2);
	}
}

void BlitLevelTiles()
{
	for(auto i : tilemap_bg)
	{
		for(auto j : i)
		{
			if(j != NULL)
				BlitTile(j, LAYER_BACKGROUND);
		}
	}
	for(auto p : tilemap_fg)
	{
		for(auto q : p)
		{
			if(q != NULL)
				BlitTile(q, LAYER_FOREGROUND);
		}
	}

	// tile coordinates
	/*for(int i = 0; i < map_height; i += 16)
	{
		SDL_Rect temp = { 0, i, level->width_in_pix, 1 };
		SDL_FillRect(surface_bg, &temp, SDL_MapRGB(surface_bg->format, 255, 255, 255));
		for(int j = 0; j < map_width; j += 50)
		{
			SDL_Surface *text_surface = TTF_RenderText_Solid(debug_font, std::to_string(i).c_str(), debug_color);
			SDL_Rect destrect = { j, i - 13, text_surface->w / RENDER_SCALE / 2, text_surface->h / RENDER_SCALE / 2};
			SDL_BlitSurface(text_surface, NULL, surface_bg, &destrect);
			SDL_FreeSurface(text_surface);
		}
	}*/
}

void GraphicsCleanup()
{
	GraphicsExit();
}

void GraphicsExit()
{
	SDL_FreeSurface(surface_bg);
	SDL_FreeSurface(surface_fg);
	SDL_FreeSurface(surface_level_textures);
	SDL_FreeSurface(debug_message);
	SDL_DestroyTexture(player_texture);
	SDL_DestroyTexture(level_bgLayer_tex);
	SDL_DestroyTexture(level_fgLayer_tex);
	textureManager.Clear();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	// close SDL_ttf
	TTF_CloseFont(debug_font);
	TTF_CloseFont(menu_font);
	TTF_CloseFont(game_font);
	TTF_CloseFont(minor_font);
	TTF_Quit();
}

void DrawHitbox(Entity &e)
{
	SDL_Rect rect;
	rect.x = (int)round(e.hitbox->GetPRect().x - camera->GetPRect().x);
	rect.y = (int)round(e.hitbox->GetPRect().y - camera->GetPRect().y);
	rect.h = (int)e.hitbox->GetPRect().h;
	rect.w = (int)e.hitbox->GetPRect().w;
	SDL_SetRenderDrawColor(renderer, 230, 0, 0, 150);
	SDL_RenderFillRect(renderer, &rect);
}

void Render(Entity &e)
{
	SDL_Rect realpos;
	double x, y;
	e.GetPos(x, y);
	realpos.x = (int)round(x - camera->GetPRect().x + e.sprite->GetSpriteOffsetX());
	realpos.y = (int)round(y - camera->GetPRect().y + e.sprite->GetSpriteOffsetY());
	realpos.h = (int)e.sprite->GetTextureCoords().h;
	realpos.w = (int)e.sprite->GetTextureCoords().w;

	realpos.x *= RENDER_SCALE;
	realpos.y *= RENDER_SCALE;
	realpos.h *= RENDER_SCALE;
	realpos.w *= RENDER_SCALE;

	if(e.status == STATUS_INVULN && e.statusTimer % 200 > 100 && e.blinkDamaged) return;

	SDL_RendererFlip flip = SDL_FLIP_NONE;
	if(e.direction == DIRECTION_LEFT)
		flip = SDL_FLIP_HORIZONTAL;
	SDL_RenderCopyEx(renderer, e.sprite->GetSpriteSheet(), &e.sprite->GetTextureCoords(), &realpos, NULL, NULL, flip);

	if(IsDebugMode)
		DrawHitbox(e);
}

void UpdateAnimation(Bullet &b)
{
	b.sprite->SetAnimation(ANIMATION_STANDING);
	b.sprite->Animate();
}

void UpdateAnimation(Effect &e)
{
	e.sprite->SetAnimation(ANIMATION_STANDING);
}

bool had_shot_while_jumping = false;
void UpdateAnimation(Player &p)
{
	if(p.charging)
	{
		if(p.charge_time == 0 && timer100.completed)
			p.ToggleChargedColor();
	}

	if(p.sprite->shootingAnimTimer > 0)
	{
		p.sprite->shootingAnimTimer -= 1;
		if(p.sprite->shootingAnimTimer <= 0)
			p.sprite->shootingAnimTimer = 0;
		else
		{
			if(p.state->Is(CREATURE_STATES::HANGING))
				p.sprite->SetAnimation(ANIMATION_SHOOTING_HANGING);
			else
			{
				if(p.state->Is(CREATURE_STATES::ONGROUND))
				{
					if(p.GetVelocity().x != 0)// && p.accel.x != 0)
						p.sprite->SetAnimation(ANIMATION_SHOOTING_RUNNING);
					else
						p.sprite->SetAnimation(ANIMATION_SHOOTING_STANDING);
				}
				else
				{
					if(p.GetVelocity().y > 0)
						p.sprite->SetAnimation(ANIMATION_SHOOTING_FALLING);
					else
					{
						p.sprite->SetAnimation(ANIMATION_SHOOTING_JUMPING);
						had_shot_while_jumping = true;
					}
				}
			}
			p.sprite->Animate();
			return;
		}
	}

	switch(p.state->GetState())
	{
		case CREATURE_STATES::SLIDING:
		{
			p.sprite->SetAnimation(ANIMATION_SLIDING);
			return;
		}
		case CREATURE_STATES::DUCKING:
		{
			p.sprite->SetAnimation(ANIMATION_DUCKING);
			return;
		}
		case CREATURE_STATES::HANGING:
		{
			p.sprite->SetAnimation(ANIMATION_HANGING);
			p.sprite->Animate();
			return;
		}
		case CREATURE_STATES::ONLADDER:
		{
			if(p.GetVelocity().y > 0 || p.GetVelocity().y < 0)
			{
				p.sprite->SetAnimation(ANIMATION_CLIMBING);
				p.sprite->Animate();
			}
			return;
		}
	}

	if(p.state->Is(CREATURE_STATES::ONGROUND))
	{
		if(p.GetVelocity().x != 0)// && p.accel.x != 0)
		{
			p.sprite->SetAnimation(ANIMATION_RUNNING);
			p.sprite->Animate();
		}
		else
		{
			p.sprite->SetAnimation(ANIMATION_STANDING);
			p.sprite->Animate();
		}
	}
	else
	{
		if(p.GetVelocity().y > 0)
		{
			p.sprite->SetAnimation(ANIMATION_FALLING);
			had_shot_while_jumping = false;
		}
		else
		{
			if(!had_shot_while_jumping)
				p.sprite->SetAnimation(ANIMATION_JUMPING);
		}
		p.sprite->Animate();
	}
	/*
		if(p.hasState(STATE_LOOKINGUP))
		{
			p.sprite->SetAnimation(6);
		}
		else
		{
			p.sprite->SetAnimation(1);
		}
	}
	if(p.hasState(STATE_DUCKING))
	{
		p.sprite->SetAnimation(13);
	}*/



	/*			if (p.status == STATUS_STUN && p.GetVelocity().y >= 0)
				{
					if (p.sprite->GetAnimation() == 9 || p.sprite->GetAnimation() == 11)
						p.sprite->SetAnimation(11);
					else
						p.sprite->SetAnimation(12);
				}
				else if(p.status != STATUS_DYING)
				{
					p.sprite->SetAnimation(0);
				}
			}
		}
		else if (p.GetVelocity().y > 0 || p.status == STATUS_STUN)
		{
			if (p.status == STATUS_STUN)
			{
				if ((p.direction && p.GetVelocity().x < 0) || (!p.direction && p.GetVelocity().x >= 0))
					p.sprite->SetAnimation(10);
				else
					p.sprite->SetAnimation(9);
			}
			else
				p.sprite->SetAnimation(4);
		}
	}*/
}

void UpdateAnimation(Creature &c)
{
	if(c.status == STATUS_DYING)
	{
		// placeholder for death animation later
		//p.sprite->UseAnimation(1);
	}
	if(c.state->Is(CREATURE_STATES::ONGROUND))
	{
		if(c.GetVelocity().x != 0)// && p.accel.x != 0)
		{
			c.sprite->SetAnimation(ANIMATION_RUNNING);
			c.sprite->Animate();
		}
		else
		{
			c.sprite->SetAnimation(ANIMATION_STANDING);
			c.sprite->Animate();
		}
	}
	else
	{
		if(c.GetVelocity().y > 0)
		{
			c.sprite->SetAnimation(ANIMATION_FALLING);
		}
		else
		{
			c.sprite->SetAnimation(ANIMATION_JUMPING);
		}
		c.sprite->Animate();
	}
}

void UpdateAnimation(Pickup &p)
{

}

void ShowDebugInfo(Player &p)
{
	char debug_str[256];
	int x, y, rectx, recty;

	p.GetPos(x, y);
	rectx = p.hitbox->GetRect().x;
	recty = p.hitbox->GetRect().y;
	int status = p.status;
	int statustimer = p.statusTimer;
	SDL_FreeSurface(debug_message);
	int tx, ty;
	tx = ConvertToTileCoord(x, false);
	ty = ConvertToTileCoord(y, false);
	sprintf(debug_str, "nearladder = %d state = %d onMachinery = %d PlayerX = %d | %d. PlayerY = %d | %d VelX: %d VelY: %d HP: %d",
		player->ammo[WEAPON_FIREBALL], p.state->GetState(), p.onMachinery, x, tx, y, ty,
		(int)p.GetVelocity().x, (int)p.GetVelocity().y, p.health);
	debug_message = TTF_RenderText_Solid(debug_font, debug_str, debug_color);

	SDL_Texture* debug_texture = SDL_CreateTextureFromSurface(renderer, debug_message);
	SDL_Rect temp;
	SDL_GetClipRect(debug_message, &temp);
	temp.x = 8;	
	temp.y = GAME_SCENE_HEIGHT - 16;
	temp.w /= RENDER_SCALE;
	temp.h /= RENDER_SCALE;
	//PrintNumToInterface(p.statusTimer, INTERFACE_SCORE, 0);
	SDL_RenderCopy(renderer, debug_texture, NULL, &temp);
	SDL_DestroyTexture(debug_texture);
}

void UpdateTransition()
{
	char *text = NULL;
	SDL_Texture *tex = NULL;
	SDL_Rect r;
	r.w = WINDOW_WIDTH;
	r.h = WINDOW_HEIGHT;
	r.x = 0;
	r.y = 0;
	SDL_SetRenderDrawColor(renderer, 204, 231, 255, 255);
	SDL_RenderFillRect(renderer, NULL);

	if(TransitionID == TRANSITION_TITLE)
	{
		SDL_RenderCopy(renderer, *textureManager.GetTexture("assets/textures/title.png"), NULL, &r);
	}
	else if(TransitionID == TRANSITION_LEVELSTART)
	{
		//if (!level->loaded)
		if(level == nullptr)
		{
			int w, h;
			text = "Generating level...";
			TTF_SizeText(menu_font, text, &w, &h);
			RenderText(GetWindowNormalizedX(0.5) - w, GetWindowNormalizedY(0.5) - h, text, menu_font, menu_color);
		}
		else
		{
			// Show level info
			int min, sec;
			min = timeLimit / 60;
			sec = timeLimit % 60;

			char str[64];
			sprintf(str, "Time: %2d min %2d sec", min, sec);
			RenderText(100, 200, str, game_font, selected_color);
			sprintf(str, "Lives left: %d", currentLives);
			RenderText(100, 250, str, game_font, selected_color);

			text = "Loaded! Press a key to start.";
			int w, h;
			TTF_SizeText(menu_font, text, &w, &h);
			RenderText(GetWindowNormalizedX(0.5) - w, 400, text, menu_font, menu_color);
		}
	}
	else if(TransitionID == TRANSITION_LEVELCLEAR)
	{
		text = "Level clear!";
		int w, h;
		TTF_SizeText(game_font, text, &w, &h);
		RenderText(GetWindowNormalizedX(0.5) - w, GetWindowNormalizedY(0.5) - h, text, game_font, menu_color);

		// TODO: probably put scoring stuff here
	}
	else if(TransitionID == TRANSITION_LEVELLOSE)
	{
		switch(gameOverReason)
		{
			case GAME_OVER_REASON_DIED:
				text = "You died!";
				break;
			case GAME_OVER_REASON_TIME:
				text = "Time's up!";
				break;
		}

		int w, h;
		TTF_SizeText(game_font, text, &w, &h);
		w = GetWindowNormalizedX(0.5) - w;
		h = GetWindowNormalizedY(0.5) - h;
		RenderText(w, h - 100, text, menu_font, menu_color);

		char str[32];
		sprintf(str, "Lives left: %d", currentLives);
		RenderText(w, h - 50, str, menu_font, menu_color);
	}
	// Render menuitems too
	if(TransitionID == TRANSITION_LEVELLOSE)
	{
		RenderMenuItems(CurrentMenu);
	}
}

void MenuUpdate()
{
	currentLives = playerLives;
}

void RenderLogo()
{
	SDL_Rect r;
	r.w = 291;
	r.h = 100;
	r.x = GetWindowNormalizedX(0.5) - r.w / 2;
	r.y = 30;
	SDL_RenderCopy(renderer, *textureManager.GetTexture("assets/textures/logo.png"), NULL, &r);
}

void ShowPauseOverlay()
{
	RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 100, "Pause", game_font, pause_color);
	RenderMenuItems(CurrentMenu);
}

void RenderMenu()
{
	SDL_SetRenderDrawColor(renderer, 204, 231, 255, 255);
	SDL_RenderFillRect(renderer, NULL);

	if(CurrentMenu == MENU_MAIN)
	{
		RenderLogo();
		SDL_Color color_credits = { 0, 0, 0 };
		RenderText(587, 480, "Outerial Studios", minor_font, color_credits);
		RenderText(550, 510, "outerial.tumblr.com", minor_font, color_credits);
		RenderText(490, 540, "Kert & MillhioreF � 2017", minor_font, color_credits);
	}
	RenderMenuItems(CurrentMenu);
	if(CurrentMenu == MENU_OPTIONS)
	{
		RenderMenuItems(MENU_SELECTION_LIVES);
	}
	if(CurrentMenu == MENU_VIDEO_OPTIONS)
	{
		RenderMenuItems(MENU_SELECTION_DISPLAY);
		RenderMenuItems(MENU_SELECTION_DISPLAY_MODE);
		RenderMenuItems(MENU_SELECTION_FULLSCREEN);
	}
	if(CurrentMenu == MENU_BINDS)
	{
		unsigned off = 40, step = 50, i = 0;
		if(menus.size() <= MENU_BINDS)
		{
			Menu *menu = new Menu();
			for(i = 0; i < NUM_CONFIGURABLE_BINDS; i++)
			{
				menu->AddMenuItem(new MenuItem(100, off + step*i, GetBindingName(i), menu_font, menu_color, selected_color));
			}
			menu->AddMenuItem(new MenuItem(100, off + step*(i++), "Reset to defaults", menu_font, menu_color, selected_color));
			menu->AddMenuItem(new MenuItem(150, off + step*i, "Back", menu_font, menu_color, selected_color));
			menus.push_back(menu);
		}
		for(i = 0; i < NUM_CONFIGURABLE_BINDS; i++)
		{
			RenderText(600, off + step*i, GetDeviceBindName(GetBindingCode(static_cast<KEYBINDS>(i))).c_str(), menu_font, menu_color);
		}
	}
	if(CurrentMenu == MENU_BIND)
	{
		// Binding key names will be most certainly updates so we delete the menu
		// It will be recreated because it won't exist. With new key names too
		while(menus.size() > MENU_BINDS)
		{
			delete menus.back();
			menus.pop_back();
		}

		RenderText(150, 180, "Press the key you wish to use for", menu_font, menu_color);
		RenderText(355, 260, GetBindingName(BindingKey), menu_font, menu_color);
		RenderText(220, 340, "(or press ESC to cancel)", menu_font, menu_color);
	}
}

void RenderMenuItems(MENUS id)
{
	Menu *menu;
	if((int)menus.size() <= id)
		return;
	menu = menus.at(id);

	for(int i = 0; i < menu->GetItemCount(); i++)
	{
		SDL_Rect r;
		SDL_Color color;
		
		if(i == SelectedItem && id == CurrentMenu || menu->IsSwitchable)
			color = menu->GetItemInfo(i)->selectedColor;
		else
			color = menu->GetItemInfo(i)->standardColor;

		if(menu->IsHorizontal && i == menu->selected)
			color = menu->GetItemInfo(i)->selectedColor;
		
		const char *text = menu->GetItemInfo(i)->text.c_str();
		r.x = menu->GetItemInfo(i)->pos.x;
		r.y = menu->GetItemInfo(i)->pos.y;
		TTF_Font *font = menu->GetItemInfo(i)->font;
		TEXT_ALIGN align = menu->GetItemInfo(i)->align;
		if(!menu->IsSwitchable || i == menu->selected)
			RenderText(r.x, r.y, text, menu->GetItemInfo(i)->font, color, align);
	}
}

void RenderText(int x, int y, std::string text, TTF_Font *font, SDL_Color color, TEXT_ALIGN align)
{
	SDL_Texture *tex = NULL;
	SDL_Surface *clearText = NULL;
	SDL_Rect r;

	clearText = TTF_RenderText_Solid(font, text.c_str(), color);
	TTF_SizeText(font, text.c_str(), &r.w, &r.h);
	r.x = x;
	r.y = y;
	switch(align)
	{
		case TEXT_ALIGN_CENTER:
		{
			r.x -= r.w / 2;
			break;
		}
		case TEXT_ALIGN_RIGHT:
		{
			r.x -= r.w;
			break;
		}
	}
	tex = SDL_CreateTextureFromSurface(renderer, clearText);
	SDL_RenderCopy(renderer, tex, NULL, &r);
	SDL_DestroyTexture(tex);
	SDL_FreeSurface(clearText);
}

void DrawFading()
{
	// clear with black
	SDL_FillRect(fading_surface, NULL, SDL_MapRGBA(fading_surface->format, 0, 0, 0, FadingVal));
	//SDL_SetRenderDrawColor(renderer, 0, 0, 0, FadingVal);
	//SDL_RenderFillRect(renderer, NULL);
	SDL_Texture *tmp;
	tmp = SDL_CreateTextureFromSurface(renderer, fading_surface);
	SDL_RenderCopy(renderer, tmp, NULL, NULL);
	SDL_DestroyTexture(tmp);
}

void WindowFlush()
{
	// clear the screen
	SDL_RenderClear(renderer);
	if(IsDebugMode)
	{
		SDL_RenderClear(maprenderer);
	}
}

void WindowUpdate()
{
	SDL_RenderPresent(renderer);
	if(IsDebugMode)
	{
		SDL_RenderPresent(maprenderer);
	}
}

void ReBlitTile(Tile *tile)
{
	SDL_Rect rect;
	SDL_Rect rect2;
	rect.x = tile->tex_x;
	rect.y = tile->tex_y;
	rect.w = TILESIZE; rect.h = TILESIZE;
	rect2.x = tile->x * TILESIZE;
	rect2.y = tile->y * TILESIZE;
	rect2.w = TILESIZE; rect2.h = TILESIZE;
	//PrintLog(LOG_SUPERDEBUG, ("x= %d y= %d ", rect2.x, rect2.y);
	//SDL_LockTexture(level_fgLayer_tex, &rect2, &surface_fg->pixels, &surface_fg->pitch);
	SDL_BlitScaled(tile->src_tex, &rect, surface_fg, &rect2);
	//SDL_UnlockTexture(level_fgLayer_tex);
}
extern std::vector<CustomTile> tileset;

void UpdateTileAnimations()
{
	for(auto &c : tileset)
	{
		if(c.animationData.sequence.size())
		{
			int numFrames = c.animationData.sequence.size();
			int currentFrame = c.animationData.currentFrame;
			int duration = c.animationData.sequence[currentFrame].duration;
			Uint32 ticks = SDL_GetTicks();
			Uint32 delta = ticks - c.animationData.timer;
			if(delta > (unsigned)duration)
			{
				c.animationData.currentFrame++;
				if(c.animationData.currentFrame >= numFrames)
					c.animationData.currentFrame = 0;
				c.animationData.timer = SDL_GetTicks();

				c.animated_x_offset = tileset[c.animationData.sequence[currentFrame].id].x_offset;
				c.animated_y_offset = tileset[c.animationData.sequence[currentFrame].id].y_offset;
			}
		}
	}

	for(auto i : tilemap_fg)
	{
		for(auto t : i)
		{
			if(t != nullptr)
			{
				if(t->HasAnimation())
				{
					if(!RENDER_ONLY_OBSERVABLE)
						UnblitTile(t);

					t->tex_x = t->customTile->animated_x_offset;
					t->tex_y = t->customTile->animated_y_offset;

					if(!RENDER_ONLY_OBSERVABLE)
						ReBlitTile(t);
				}
			}
		}
	}
}

SDL_Texture* GenerateLightningTexture(std::vector<SDL_Point> &points)
{
	int width = points.back().x;
	SDL_Surface *lightning = SDL_CreateRGBSurface(0, width, 3 + 20, 32,
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000);
	SDL_Rect src;
	SDL_GetClipRect(lightningSegment, &src);
	for(int i = 0; i < (int)points.size(); i++)
	{
		SDL_Point *p = &points[i];
		SDL_Rect clone = src;
		if(p->y > 512)
		{
			p->y -= 512;
			clone.h = 1;
		}
		else if(p->y > 256)
		{
			p->y -= 256;
			clone.h = 2;
		}
		SDL_Rect dest = { p->x, p->y - 1, 1, 3 };
		SDL_BlitSurface(lightningSegment, &clone, lightning, &dest);
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, lightning);
	SDL_FreeSurface(lightning);
	return tex;
}

void SetupLevelGraphics(int map_width, int map_height)
{
	surface_bg = SDL_CreateRGBSurface(0, map_width, map_height, 32,
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000);
	surface_fg = SDL_CreateRGBSurface(0, map_width, map_height, 32,
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000);
	SDL_SetColorKey(surface_bg, 1, SDL_MapRGB(surface_bg->format, 0, 0, 0));
	SDL_SetColorKey(surface_fg, 1, SDL_MapRGB(surface_fg->format, 0, 0, 0));
	BlitLevelTiles();
	mapoverview_texture = SDL_CreateTextureFromSurface(maprenderer, surface_bg);
	if(RENDER_ONLY_OBSERVABLE)
	{
		SDL_FreeSurface(surface_bg);
		SDL_FreeSurface(surface_fg);
		surface_bg = nullptr;
		surface_fg = nullptr;
	}
	else
	{
		level_bgLayer_tex = SDL_CreateTextureFromSurface(renderer, surface_bg);
		level_fgLayer_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, map_width, map_height);
		SDL_SetTextureBlendMode(level_fgLayer_tex, SDL_BLENDMODE_BLEND);
	}
}

int GetWindowNormalizedX(double val)
{
	val = std::max(0.0, val);
	val = std::min(100.0, val);
	return WINDOW_WIDTH * val;
}

int GetWindowNormalizedY(double val)
{
	val = std::max(0.0, val);
	val = std::min(100.0, val);
	return WINDOW_HEIGHT * val;
}