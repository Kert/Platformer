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

Camera* camera;
extern Player *player;

bool graphicsLoaded = false;

extern int TransitionID;
extern MENUS CurrentMenu;
extern std::map<MENUS, Menu*> menus;
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

static int const MAX_TILES_VERTICALLY = 18;

int WINDOW_WIDTH;
int WINDOW_HEIGHT;

// SHOULD CALCULATE THIS
int RENDER_SCALE = 1;

// in non-scaled pixels 
int GAME_SCENE_WIDTH;
int GAME_SCENE_HEIGHT;

SDL_Renderer *renderer = NULL;
SDL_Surface *player_surf = NULL;
SDL_Texture *player_texture = NULL;
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
SDL_Color pause_color = { 255, 255, 255 };
SDL_Color menu_color = { 255, 255, 255 };
SDL_Color selected_color = { 0, 255, 0 };
SDL_Surface *debug_message = NULL;

extern std::vector<std::vector<std::vector<Tile*>>> tileLayers;

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

	debug_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 8);
	menu_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 32);
	game_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 32);
	minor_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 16);

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
	
	RENDER_SCALE = static_cast<int>(ceil((double)WINDOW_HEIGHT / (double)MAX_TILES_VERTICALLY / (double)(TILESIZE)));

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
		MenusCleanup();
		LoadMenus();
		return;
	}
	
	if(!displayModes.count(displayIndex))
	{
		PrintLog(LOG_IMPORTANT, "Display %i doesn't exist. Defaulting to 0", displayIndex);
		displayIndex = 0;
	}

	SDL_DisplayMode mode;
	if(displayMode.format == 0) // config is missing
	{
		mode = displayModes[0][0]; // first available
	}
	else
	{
		for(auto i : displayModes[displayIndex])
		{
			if(i.h == displayMode.h &&
				i.w == displayMode.w &&
				i.refresh_rate == displayMode.refresh_rate &&
				i.format == displayMode.format)
			{
				mode = i;
				break;
			}
		}
	}
	SetDisplayMode(mode);
	// TODO: Check for leaks
	MenusCleanup();
	LoadMenus();
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
	SDL_BlitSurface(tile->src_tex, &rect, pov_surface, &rect2);
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
				for(auto &layer : tileLayers)
				{
					BlitObserveTileAt(layer[i][j], p, q);
				}
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

	BlitObservableTiles();
	
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
}

void GraphicsCleanup()
{
	GraphicsExit();
}

void GraphicsExit()
{
	SDL_FreeSurface(surface_level_textures);
	SDL_FreeSurface(debug_message);
	SDL_DestroyTexture(player_texture);
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
	temp.x = 8 * RENDER_SCALE;
	temp.y = (GAME_SCENE_HEIGHT - 16) * RENDER_SCALE;
	//PrintNumToInterface(p.statusTimer, INTERFACE_SCORE, 0);
	SDL_RenderCopy(renderer, debug_texture, NULL, &temp);
	SDL_DestroyTexture(debug_texture);

	SDL_Rect virtualCamRect;
	virtualCamRect.x = (camera->virtualCam.x - camera->GetRect().x) * RENDER_SCALE;
	virtualCamRect.y = (camera->virtualCam.y - camera->GetRect().y) * RENDER_SCALE;
	virtualCamRect.w = camera->virtualCam.w * RENDER_SCALE;
	virtualCamRect.h = camera->virtualCam.h * RENDER_SCALE;
	SDL_SetRenderDrawColor(renderer, 0, 200, 10, 250);
	SDL_RenderDrawRect(renderer, &virtualCamRect);	
}

void RenderTransition()
{
	char *text = NULL;
	SDL_Texture *tex = NULL;
	SDL_Rect r;
	r.w = WINDOW_WIDTH;
	r.h = WINDOW_HEIGHT;
	r.x = 0;
	r.y = 0;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
			RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "LOADING LEVEL...", menu_font, menu_color, TEXT_ALIGN_CENTER);
		}
		else
		{
			// Show level info
			int min, sec;
			min = timeLimit / 60;
			sec = timeLimit % 60;

			char str[64];
			sprintf(str, "Time: %2d min %2d sec", min, sec);
			RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.2), str, game_font, selected_color, TEXT_ALIGN_CENTER);
			sprintf(str, "Lives left: %d", currentLives);
			RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.2) + 100, str, game_font, selected_color, TEXT_ALIGN_CENTER);

			RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "LOADED! PRESS A KEY TO START", menu_font, menu_color, TEXT_ALIGN_CENTER);
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

void RenderMenu()
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(renderer, NULL);

	switch(CurrentMenu)
	{
		case MENU_MAIN:
		{
			RenderLogo();
			SDL_Color color_credits = { 0, 0, 0 };
			RenderText(587, 480, "Outerial Studios", minor_font, color_credits);
			RenderText(550, 510, "outerial.tumblr.com", minor_font, color_credits);
			RenderText(490, 540, "Kert & MillhioreF © 2017", minor_font, color_credits);
			break;
		}
		case MENU_MAPSELECT:
		{
			RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.1), "SELECT LEVEL", menu_font, menu_color, TEXT_ALIGN_CENTER);
			break;
		}
		case MENU_OPTIONS:
		{
			RenderMenuItems(MENU_SELECTION_MUSIC_VOLUME);
			break;
		}
		case MENU_VIDEO_OPTIONS:
		{
			RenderMenuItems(MENU_SELECTION_DISPLAY);
			RenderMenuItems(MENU_SELECTION_DISPLAY_MODE);
			RenderMenuItems(MENU_SELECTION_FULLSCREEN);
			break;
		}
		case MENU_BINDS:
		{
			unsigned off = 40, step = 50, i = 0;
			if(menus.find(MENU_BINDS) == menus.end())
			{
				Menu *menu = new Menu();
				for(i = 0; i < NUM_CONFIGURABLE_BINDS; i++)
				{
					menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5) - 256, off + step*i, GetBindingName(i), menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
				}
				menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 200, "RESET TO DEFAULTS", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
				menu->AddMenuItem(new MenuItem(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.4) + 300, "BACK", menu_font, menu_color, selected_color, TEXT_ALIGN_CENTER));
				menus[MENU_BINDS] = menu;
			}
			for(i = 0; i < NUM_CONFIGURABLE_BINDS; i++)
			{
				RenderText(GetWindowNormalizedX(0.5) + 256, off + step*i, GetDeviceBindName(GetBindingCode(static_cast<KEYBINDS>(i))).c_str(), menu_font, menu_color, TEXT_ALIGN_CENTER);
			}
			break;
		}
		case MENU_BINDKEY:
		{
			RenderText(GetWindowNormalizedX(0.5), 180, "PRESS THE KEY YOU WISH TO USE FOR", menu_font, menu_color, TEXT_ALIGN_CENTER);
			RenderText(GetWindowNormalizedX(0.5), 260, GetBindingName(BindingKey), menu_font, selected_color, TEXT_ALIGN_CENTER);
			RenderText(GetWindowNormalizedX(0.5), 340, "(OR PRESS ESC TO CANCEL)", menu_font, menu_color, TEXT_ALIGN_CENTER);
			break;
		}
	}

	RenderMenuItems(CurrentMenu);
}

void RenderMenuItems(MENUS id)
{
	Menu *menu;
	if(menus.find(id) == menus.end())
		return;
	menu = menus.at(id);

	for(int i = 0; i < menu->GetItemCount(); i++)
	{
		if(!menu->IsSwitchable || i == menu->selected)
		{			
			SDL_Color color;
			if(i == SelectedItem && id == CurrentMenu || menu->IsSwitchable)
				color = menu->GetItemInfo(i)->selectedColor;
			else
				color = menu->GetItemInfo(i)->standardColor;
			if(menu->IsHorizontal && i == menu->selected)
				color = menu->GetItemInfo(i)->selectedColor;
						
			SDL_Rect r;
			r.x = menu->GetItemInfo(i)->pos.x;
			r.y = menu->GetItemInfo(i)->pos.y;
			TTF_Font *font = menu->GetItemInfo(i)->font;
			TEXT_ALIGN align = menu->GetItemInfo(i)->align;
			const char *text = menu->GetItemInfo(i)->text.c_str();
			RenderText(r.x, r.y, text, menu->GetItemInfo(i)->font, color, align);
		}
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
}

void WindowUpdate()
{
	SDL_RenderPresent(renderer);
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

	for(auto &layer : tileLayers)
	{
		for(auto &i : layer)
		{
			for(auto t : i)
			{
				if(t != nullptr)
				{
					if(t->HasAnimation())
					{
						t->tex_x = t->customTile->animated_x_offset;
						t->tex_y = t->customTile->animated_y_offset;
					}
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