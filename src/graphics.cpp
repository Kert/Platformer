#include "graphics.h"
#include <SDL_image.h>
#include <iomanip>
#include <sstream>
#include "animation.h"
#include "camera.h"
#include "entities.h"
#include "gamelogic.h"
#include "interface.h"
#include "level.h"
#include "state.h"
#include "tiles.h"
#include "menu.h"
#include "transition.h"
#include "utils.h"

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
	SDL_Texture *tex = IMG_LoadTexture(Graphics::GetRenderer(), filename.c_str());
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

// TODO: reorganize this
extern std::map<std::string, CreatureData> creatureData;
extern std::map<std::string, EntityGraphicsData> entityGraphicsData;

extern std::vector<Bullet*> bullets;
extern std::vector<Effect*> effects;
extern std::vector<Creature*> creatures;
extern std::vector<Pickup*> pickups;
extern std::vector<Machinery*> machinery;
extern std::vector<Lightning*> lightnings;
extern std::vector<TileLayerData> tileLayers;
extern std::vector<CustomTile> tileset;

SDL_Color debug_color = { 50, 180, 0 };
SDL_Color pause_color = { 255, 255, 255 };
SDL_Color menu_color = { 255, 255, 255 };
SDL_Color selected_color = { 0, 255, 0 };

namespace Graphics
{
	bool graphicsLoaded = false;
	std::map<int, std::vector<SDL_DisplayMode>> displayModes;

	SDL_DisplayMode displayMode;
	int displayIndex;
	int fullscreenMode;
	SCALING_MODES scalingMode;

	Timer timer100{ 100 }, timerRain{ 200 };

	std::vector<Timer*> TimersGraphics{ &timer100, &timerRain };
	
	int const MAX_TILES_VERTICALLY = 15;

	// Calculated based on resolution and scaling type
	double RENDER_SCALE = 1;

	// in non-scaled pixels 
	int GAME_SCENE_WIDTH;
	int GAME_SCENE_HEIGHT;

	SDL_Renderer *renderer = NULL;
	SDL_Surface *player_surface = NULL;
	SDL_Texture *level_texture = NULL;
	SDL_Surface *lightningSegment = NULL;

	SDL_Texture *unscaled_scene = nullptr;
	SDL_Texture *scaled_scene = nullptr;

	SDL_Window *win = NULL;

	TTF_Font *debug_font = NULL;
	TTF_Font *menu_font = NULL;
	TTF_Font *game_font = NULL;
	TTF_Font *minor_font = NULL;
	TTF_Font *interface_font = NULL;

	RandomGenerator graphics_rg;
	struct ScreenShake
	{
		double timer = 0;
		int offsetX;
		int offsetY;
	} screenShake;

	Camera* camera;

	int FindDisplayModes();
	void DrawVirtualCamera();

	Camera* GetCamera()
	{
		return camera;
	}

	void CreateCamera()
	{
		camera = new Camera(0, 0, GetGameSceneWidth(), GetGameSceneHeight());
	}

	void RemoveCamera()
	{
		delete camera;
		camera = nullptr;
	}

	void InitPlayerTexture()
	{
		std::string textureName = entityGraphicsData[creatureData["Player"].graphicsName].textureFile;
		// Load the image
		if(!player_surface)
			player_surface = IMG_Load(textureName.c_str());

		// Replace texture in the manager texture array
		SDL_Texture **ptr = textureManager.GetTexture(textureName.c_str());
		if(*ptr)
			SDL_DestroyTexture(*ptr);
		*ptr = SDL_CreateTextureFromSurface(renderer, player_surface);
	}

	void ChangePlayerColor(PLAYER_BODY_PARTS bodyPart, SDL_Color color)
	{
		player_surface->format->palette->colors[bodyPart] = color;
		InitPlayerTexture();
	}

	int Init()
	{
		if(graphicsLoaded) return 0;

		// Initialize SDL_TTF for font rendering
		if(TTF_Init() == -1)
			return 0;

		debug_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 8);
		menu_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 32);
		game_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 32);
		minor_font = TTF_OpenFont("assets/misc/PressStart2P.ttf", 8);
		interface_font = minor_font;

		// create the window and renderer
		// note that the renderer is accelerated
		win = SDL_CreateWindow("Platformer", 100, 100, 640, 480, NULL);
		renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		SDL_Surface *icon = IMG_Load("assets/misc/icon.png");
		SDL_SetWindowIcon(win, icon);
		SDL_FreeSurface(icon);

		FindDisplayModes();

		UpdateDisplayMode();

		// Allows drawing half-transparent rectangles
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		InterfaceSetup();

		lightningSegment = IMG_Load("assets/textures/millhilightning.png");

		graphicsLoaded = true;
		return 1;
	}

	int FindDisplayModes()
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
		displayMode = mode;
		if(GetFullscreenMode() != SDL_WINDOW_FULLSCREEN)
		{
			SDL_DisplayMode desktopMode;
			SDL_GetDesktopDisplayMode(displayIndex, &desktopMode);
			displayMode.refresh_rate = desktopMode.refresh_rate;
		}
		int WINDOW_WIDTH = mode.w;
		int WINDOW_HEIGHT = mode.h;

		SDL_SetWindowSize(win, WINDOW_WIDTH, WINDOW_HEIGHT);
		SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		switch(scalingMode)
		{			
			case SCALING_ADAPTIVE:
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
				RENDER_SCALE = static_cast<int>(ceil((double)WINDOW_HEIGHT / (double)MAX_TILES_VERTICALLY / (double)(TILESIZE)));
				break;
			case SCALING_LETTERBOXED:
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
				RENDER_SCALE = static_cast<int>(floor((double)WINDOW_HEIGHT / (double)MAX_TILES_VERTICALLY / (double)(TILESIZE)));
				break;
			default:
				RENDER_SCALE = WINDOW_HEIGHT / (double)MAX_TILES_VERTICALLY / (double)(TILESIZE);
		}

		GAME_SCENE_WIDTH = static_cast<int>(ceil((double)WINDOW_WIDTH / RENDER_SCALE));
		GAME_SCENE_HEIGHT = static_cast<int>(ceil((double)WINDOW_HEIGHT / RENDER_SCALE));

		if(unscaled_scene)
			SDL_DestroyTexture(unscaled_scene);
		if(scaled_scene)
		{
			SDL_DestroyTexture(scaled_scene);
			scaled_scene = nullptr;
		}
		unscaled_scene = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, GAME_SCENE_WIDTH, GAME_SCENE_HEIGHT);

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
			SetDisplayMode(displayMode);
			SDL_SetWindowDisplayMode(win, &displayMode);
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
		SDL_SetWindowDisplayMode(win, &mode);
		// TODO: Check for leaks
		MenusCleanup();
		LoadMenus();
	}

	void DrawFPS(long long dt)
	{
		long long fps = 1000000000 / dt;
		RenderText(10, 10, "fps: " + std::to_string(fps), debug_font, debug_color);
	}

	void BlitObserveTileAt(Tile* tile, int x, int y)
	{
		if(tile == NULL) return;
		SDL_Rect rect;
		SDL_Rect rect2;

		rect.x = tile->tex_x;
		rect.y = tile->tex_y;
		rect.w = TILESIZE; rect.h = TILESIZE;

		rect2.x = x + screenShake.offsetX;
		rect2.y = y + screenShake.offsetY;
		rect2.w = rect.w;
		rect2.h = rect.h;

		//PrintLog(LOG_SUPERDEBUG, ("x= %d y= %d ", rect2.x, rect2.y);
		SDL_RenderCopy(renderer, tile->src_tex, &rect, &rect2);
	}

	void BlitObservableTiles()
	{
		Level *level = Game::GetLevel();
		// clear with background color
		SDL_SetRenderDrawColor(renderer, level->bgColor.r, level->bgColor.g, level->bgColor.b, 255);
		SDL_RenderFillRect(renderer, NULL);

		int w, h;
		PrecisionRect prect = camera->GetPRect();
		SDL_Rect virtCam = camera->GetVirtualCamRect();

		w = ConvertToTileCoord(prect.w, true);
		h = ConvertToTileCoord(prect.h, true);
		if(scalingMode == SCALING_LETTERBOXED)
		{
			w = ConvertToTileCoord(virtCam.w, false);
			h = ConvertToTileCoord(virtCam.h, false);
		}

		int a = SDL_GetTicks();
		// counters for current tile pos to blit to
		for(int layerIndex = 0; layerIndex < (int)tileLayers.size(); layerIndex++)
		{
			TileLayerData *curLayer = &tileLayers[layerIndex];
			double parallaxDepthX = curLayer->parallaxDepthX;
			int parallaxOffsetX = curLayer->parallaxOffsetX;
			int actualX = (int)floor(parallaxOffsetX * parallaxDepthX * TILESIZE + virtCam.x * parallaxDepthX);
			if(scalingMode != SCALING_LETTERBOXED)
				actualX -= (int)floor(virtCam.x - prect.x);
			actualX /= TILESIZE;
			for(int i = actualX; i <= actualX + w; i++)
			{
				double parallaxDepthY = curLayer->parallaxDepthY;
				int parallaxOffsetY = curLayer->parallaxOffsetY;
				int actualY = (int)floor(parallaxOffsetY * parallaxDepthY * TILESIZE + virtCam.y * parallaxDepthY);
				if(scalingMode != SCALING_LETTERBOXED)
					actualY -= (int)floor(virtCam.y - prect.y);
				actualY /= TILESIZE;
				for(int j = actualY; j <= actualY + h; j++)
				{
					if(i >= 0 && j >= 0 && i < level->width_in_tiles && j < level->height_in_tiles)
					{
						int p = (int)floor(i * TILESIZE - virtCam.x * parallaxDepthX - parallaxOffsetX * TILESIZE * parallaxDepthX + (virtCam.x - prect.x));
						int q = (int)floor(j * TILESIZE - virtCam.y * parallaxDepthY - parallaxOffsetY * TILESIZE * parallaxDepthY + (virtCam.y - prect.y));

						Tile *tile = curLayer->tiles[i][j];
						if(tile != nullptr)
						{
							tile->Animate();
							BlitObserveTileAt(tile, p, q);
						}						
					}
				}
			}
		}
		int b = SDL_GetTicks();
		//PrintLog(LOG_DEBUG, "%d time passed", b - a);
	}

	void Update(double ticks)
	{
		for(auto &t : TimersGraphics)
		{
			t->Run();
		}
		
		SDL_SetRenderTarget(renderer, unscaled_scene);

		UpdateTileAnimations();

		ScreenShakeUpdate(ticks);
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
		Player *player = Game::GetPlayer();
		UpdateAnimation(*player, ticks);
		Render(*player);

		for(auto &e : effects)
		{
			UpdateAnimation(*e);
			Render(*e);
		}

		for(auto &b : bullets)
		{
			UpdateAnimation(*b);
			Render(*b);
		}		

		RenderInterface();

		if(scalingMode == SCALING_LETTERBOXED)
			DrawLetterbox();

		if(Game::IsDebug()) DrawVirtualCamera();

		// don't use upscaling+linear downscaling for integer render scale and default scaling mode
		if(scalingMode == SCALING_DEFAULT && (floor(RENDER_SCALE) != RENDER_SCALE))
		{
			if(!scaled_scene)
			{
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
				scaled_scene = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, GAME_SCENE_WIDTH * 4, GAME_SCENE_HEIGHT * 4);
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
			}
			SDL_SetRenderTarget(renderer, scaled_scene);
			SDL_RenderCopy(renderer, unscaled_scene, NULL, NULL);

			SDL_SetRenderTarget(renderer, NULL);
			SDL_RenderCopy(renderer, scaled_scene, NULL, NULL);
		}
		else
		{
			SDL_SetRenderTarget(renderer, NULL);
			SDL_Rect dest;
			dest.x = dest.y = 0;
			dest.w = GAME_SCENE_WIDTH * RENDER_SCALE;
			dest.h = GAME_SCENE_HEIGHT * RENDER_SCALE;
			SDL_RenderCopy(renderer, unscaled_scene, NULL, &dest);
		}

		if(Game::IsDebug()) ShowDebugInfo(*player);
	}

	void Cleanup()
	{
		SDL_DestroyTexture(unscaled_scene);
		SDL_DestroyTexture(scaled_scene);
		SDL_DestroyTexture(level_texture);
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
		rect.x = (int)(e.hitbox->GetPRect().x - camera->GetPRect().x);
		rect.y = (int)(e.hitbox->GetPRect().y - camera->GetPRect().y);
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
		realpos.x = (int)(x - camera->GetPRect().x + e.sprite->GetSpriteOffsetX());
		realpos.y = (int)(y - camera->GetPRect().y + e.sprite->GetSpriteOffsetY());
		realpos.h = (int)e.sprite->GetTextureCoords().h;
		realpos.w = (int)e.sprite->GetTextureCoords().w;

		if(e.status == STATUS_INVULN && SDL_GetTicks() % 200 > 100 && e.blinkDamaged) return;

		SDL_RendererFlip flip = SDL_FLIP_NONE;
		if(e.direction == DIRECTION_LEFT)
			flip = SDL_FLIP_HORIZONTAL;
		SDL_RenderCopyEx(renderer, e.sprite->GetSpriteSheet(), &e.sprite->GetTextureCoords(), &realpos, NULL, NULL, flip);

		if(Game::IsDebug())
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
		e.sprite->Animate();
	}

	bool had_shot_while_jumping = false;
	void UpdateAnimation(Player &p, double ticks)
	{
		if(p.charging)
		{
			if(p.charge_time == 0 && timer100.completed)
				p.ToggleChargedColor();
		}

		if(!(p.state->Is(CREATURE_STATES::ONGROUND) && p.GetVelocity().x == 0))
			p.idleTimer = 0;

		if(p.sprite->shootingAnimTimer > 0)
		{
			p.idleTimer = 0;
			p.sprite->shootingAnimTimer -= ticks;
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
				if(p.idleTimer > 500)
				{
					p.sprite->SetAnimation(ANIMATION_IDLE);
				}
				else
				{					
					p.sprite->SetAnimation(ANIMATION_STANDING);
					p.idleTimer++;
				}
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
		int x, y, rectx, recty;
		p.GetPos(x, y);
		rectx = p.hitbox->GetRect().x;
		recty = p.hitbox->GetRect().y;
		Velocity vel = p.GetVelocity();
		int tx, ty;
		tx = ConvertToTileCoord(x, false);
		ty = ConvertToTileCoord(y, false);
		
		std::ostringstream debug_str;
		debug_str << "X: " << x << " | " << tx
			<< " Y: " << y << " | " << ty
			<< " VelX: " << vel.x << " VelY: " << vel.y
			<< " AccX: " << p.accel.x << " AccY: " << p.accel.y
			<< " Attached X: " << p.attX << " Y: " << p.attY;
		RenderText(0, GetWindowNormalizedY(1) - 32, debug_str.str(), debug_font, debug_color);
		
		debug_str.str("");
		debug_str.clear();
		debug_str << "HP: " << p.health
			<< " state: " << p.state->GetState()
			<< " onMachinery: " << p.onMachinery
			<< " status: " << p.status
			<< " statusTimer: " << p.statusTimer
			<< " doubleJumped: " << p.doubleJumped
			<< " jumpTime: " << p.jumptime;

		RenderText(0, GetWindowNormalizedY(1) - 16, debug_str.str(), debug_font, debug_color);
	}

	void RenderInterface()
	{
		int healthFrame = ((100 - Game::GetPlayer()->health) / 25);
		ChangeInterfaceFrame(healthFrame, INTERFACE_LIFE);

		SDL_Color interface_color = { 50, 180, 0 };
		std::map<int, InterfacePiece> *interfaces = GetInterfaces();
		for(auto iter : *interfaces)
		{
			int posX, posY;
			if(scalingMode == SCALING_LETTERBOXED)
			{
				SDL_Rect virtCam = camera->GetVirtualCamRect();
				posX = (iter.second.location.x + (virtCam.x - camera->GetRect().x));
				posY = (iter.second.location.y + (virtCam.y - camera->GetRect().y));
			}
			else
			{
				posX = iter.second.location.x;
				posY = iter.second.location.y;
			}
			if(iter.second.tex == NULL)	
				RenderText(posX, posY, iter.second.text, interface_font, interface_color);
			else
			{
				SDL_Rect dest;
				dest.x = posX;
				dest.y = posY;
				dest.w = iter.second.location.w;
				dest.h = iter.second.location.h;
				SDL_RenderCopy(GetRenderer(), iter.second.tex, &iter.second.frame, &dest);
			}
		}
	}

	void RenderTransition()
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, NULL);

		TRANSITIONS transition = GetCurrentTransition();
		switch(transition)
		{
			case TRANSITION_TITLE:
				SDL_RenderCopy(renderer, *textureManager.GetTexture("assets/textures/title.png"), NULL, NULL);
				break;
			case TRANSITION_LEVELSTART:
				//if (!level->loaded)
				if(Game::GetLevel() == nullptr)
				{
					RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "LOADING LEVEL...", menu_font, menu_color, TEXT_ALIGN_CENTER);
				}
				else
				{
					// Show level info
					int min, sec;
					min = Game::GetTimeLimit() / 60;
					sec = Game::GetTimeLimit() % 60;

					std::ostringstream msg;
					msg << "TIME: " << std::setw(2) << min << " MIN " << sec << " SEC";
					RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.2), msg.str(), game_font, selected_color, TEXT_ALIGN_CENTER);
					
					msg.str("");
					msg.clear();
					msg << "LIVES LEFT: " << Game::GetPlayerLivesLeft();
					RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.2) + 100, msg.str(), game_font, selected_color, TEXT_ALIGN_CENTER);

					RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "LOADED! PRESS A KEY TO START", menu_font, menu_color, TEXT_ALIGN_CENTER);
				}
				break;
			case TRANSITION_LEVELCLEAR:
				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), "LEVEL CLEAR!", game_font, menu_color, TEXT_ALIGN_CENTER);
				break;
		}
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

		MENUS CurrentMenu = GetCurrentMenu();
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
			case MENU_SOUND_OPTIONS:
			{
				RenderMenuItems(MENU_SELECTION_MUSIC_VOLUME);
				RenderMenuItems(MENU_SELECTION_SFX_VOLUME);
				break;
			}
			case MENU_VIDEO_OPTIONS:
			{
				RenderMenuItems(MENU_SELECTION_DISPLAY);
				RenderMenuItems(MENU_SELECTION_DISPLAY_MODE);
				RenderMenuItems(MENU_SELECTION_FULLSCREEN);
				RenderMenuItems(MENU_SELECTION_SCALING_MODE);
				break;
			}
			case MENU_BINDS:
			{
				RenderText(GetWindowNormalizedX(0.3) + 32 * 2, GetMenus()->at(MENU_BINDS)->GetItemInfo(0)->pos.y - 32 * 2, "KEYBOARD", menu_font, menu_color, TEXT_ALIGN_LEFT);
				RenderText(GetWindowNormalizedX(0.3) + 32 * 13, GetMenus()->at(MENU_BINDS)->GetItemInfo(0)->pos.y - 32 * 2, "GAMEPAD", menu_font, menu_color, TEXT_ALIGN_LEFT);
				int i = 0;
				std::vector<KEYBINDS> bindables = GetBindables();
				for(auto bind : bindables)
				{
					int x = GetWindowNormalizedX(0.3) + 32 * 2;
					int y = GetMenus()->at(MENU_BINDS)->GetItemInfo(i)->pos.y;
					RenderText(x, y, GetKeyboardKeyName(GetKeyboardCodeFromBind(bind)).c_str(), menu_font, menu_color, TEXT_ALIGN_LEFT);
					x += 32 * 11;
					RenderText(x, y, GetControllerKeyName(GetControllerCodeFromBind(bind)).c_str(), menu_font, menu_color, TEXT_ALIGN_LEFT);
					i++;
				}
				break;
			}
			case MENU_BINDKEY:
			{
				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 3, "PRESS THE KEY YOU WISH TO USE FOR", menu_font, menu_color, TEXT_ALIGN_CENTER);
				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5), GetBindingName(GetCurrentKeyToBind()), menu_font, selected_color, TEXT_ALIGN_CENTER);
				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) + 32 * 3, "(OR PRESS ESC TO CANCEL)", menu_font, menu_color, TEXT_ALIGN_CENTER);
				break;
			}
			case MENU_PLAYER_FAILED: case MENU_PLAYER_FAILED_NO_ESCAPE:
			{
				std::string text;
				switch(Game::GetGameOverReason())
				{
					case GAME_OVER_REASON_DIED:
						text = "YOU DIED!";
						break;
					case GAME_OVER_REASON_TIME:
						text = "TIME'S UP!";
						break;
				}

				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 9, text, menu_font, menu_color, TEXT_ALIGN_CENTER);

				text = "LIVES LEFT: " + std::to_string(Game::GetPlayerLivesLeft());
				RenderText(GetWindowNormalizedX(0.5), GetWindowNormalizedY(0.5) - 32 * 7, text, menu_font, menu_color, TEXT_ALIGN_CENTER);
				break;
			}
		}

		RenderMenuItems(CurrentMenu);
	}

	void RenderMenuItems(MENUS id)
	{
		Menu *menu;
		if(GetMenus()->find(id) == GetMenus()->end())
			return;
		menu = GetMenus()->at(id);

		for(int i = 0; i < menu->GetItemCount(); i++)
		{
			if(menu->IsSwitchable && i != menu->selected)
				continue;

			SDL_Color color;

			if(i == menu->selected)
				color = menu->GetItemInfo(i)->selectedColor;
			else
				color = menu->GetItemInfo(i)->standardColor;

			SDL_Rect r;
			r.x = menu->GetItemInfo(i)->pos.x;
			r.y = menu->GetItemInfo(i)->pos.y;
			TTF_Font *font = GetFont(menu->GetItemInfo(i)->font);
			TEXT_ALIGN align = menu->GetItemInfo(i)->align;
			std::string text = menu->GetItemInfo(i)->text;
			RenderText(r.x, r.y, text, font, color, align);
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
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, Fading::GetVal());
		SDL_RenderFillRect(renderer, NULL);
	}

	void WindowFlush()
	{
		SDL_RenderClear(renderer);
	}

	void WindowUpdate()
	{
		SDL_RenderPresent(renderer);
	}

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
		int width, height;
		SDL_GetWindowSize(win, &width, &height);
		return width * val;
	}

	int GetWindowNormalizedY(double val)
	{
		val = std::max(0.0, val);
		val = std::min(100.0, val);
		int width, height;
		SDL_GetWindowSize(win, &width, &height);
		return height * val;
	}
	
	int GetFullscreenMode()
	{
		return fullscreenMode;
	}

	void SetFullscreenMode(int mode)
	{
		fullscreenMode = mode;
	}

	SDL_DisplayMode GetDisplayMode()
	{
		return displayMode;
	}

	SDL_DisplayMode GetDisplayMode(int dispIndex, int modeIndex)
	{
		return displayModes[dispIndex][modeIndex];
	}

	std::map<int, std::vector<SDL_DisplayMode>> GetDisplayModes()
	{
		return displayModes;
	}

	int GetDisplayIndex()
	{
		return displayIndex;
	}

	void SetDisplayIndex(int index)
	{
		displayIndex = index;
	}

	int GetRefreshRate()
	{
		return GetDisplayMode().refresh_rate;
	}

	int GetGameSceneWidth()
	{
		return GAME_SCENE_WIDTH;
	}

	int GetGameSceneHeight()
	{
		return GAME_SCENE_HEIGHT;
	}

	SDL_Renderer* GetRenderer()
	{
		return renderer;
	}

	int LoadLevelTexturesFromFile(std::string fileName)
	{
		level_texture = IMG_LoadTexture(renderer, fileName.c_str());
		return 0;
	}

	SDL_Texture* GetLevelTexture()
	{
		return level_texture;
	}

	TTF_Font* GetFont(FONTS font)
	{
		switch(font)
		{
			case FONT_GAME:
				return game_font;
			case FONT_MINOR:
				return minor_font;
			case FONT_MENU:
				return menu_font;
			case FONT_INTERFACE:
				return interface_font;
			case FONT_DEBUG:
				return debug_font;
		}
	}

	void ScreenShake(double sec)
	{
		screenShake.timer = SecToTicks(1);
	}

	void ScreenShakeUpdate(double ticks)
	{
		if(screenShake.timer <= 0)
		{
			screenShake.offsetX = 0;
			screenShake.offsetY = 0;
			return;
		}
				
		// TODO: Don't change it each frame maybe?
		int randVals[] = { 0, -8, 8 };
		int rand = graphics_rg.Generate(0, sizeof(randVals) / sizeof(randVals[0]));
		screenShake.offsetX = rand;
		rand = graphics_rg.Generate(0, sizeof(randVals) / sizeof(randVals[0]));
		screenShake.offsetY = rand;
		screenShake.timer -= ticks;
	}

	void DrawLetterbox()
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_Rect rectToDraw;
		SDL_Rect virtCam = camera->GetVirtualCamRect();
		// top
		rectToDraw.x = rectToDraw.y = 0;
		rectToDraw.w = GAME_SCENE_WIDTH;
		rectToDraw.h = virtCam.y - camera->GetPRect().y;
		SDL_RenderFillRect(renderer, &rectToDraw);
		// bottom
		rectToDraw.x = 0;
		rectToDraw.y = rectToDraw.h + virtCam.h;
		rectToDraw.h = GAME_SCENE_HEIGHT - rectToDraw.y;
		SDL_RenderFillRect(renderer, &rectToDraw);
		// left
		rectToDraw.x = rectToDraw.y = 0;
		rectToDraw.h = GAME_SCENE_HEIGHT;
		rectToDraw.w = virtCam.x - camera->GetPRect().x;
		SDL_RenderFillRect(renderer, &rectToDraw);
		// right
		rectToDraw.y = 0;
		rectToDraw.x = rectToDraw.w + virtCam.w;
		rectToDraw.w = GAME_SCENE_WIDTH - rectToDraw.w;
		SDL_RenderFillRect(renderer, &rectToDraw);		
	}

	SCALING_MODES GetScalingMode()
	{
		return scalingMode;
	}

	void SetScalingMode(int mode)
	{
		scalingMode = (SCALING_MODES)mode;
	}

	void DrawVirtualCamera()
	{
		SDL_Rect virtCamToDraw;
		SDL_Rect virtCam = camera->GetVirtualCamRect();
		virtCamToDraw.x = (virtCam.x - camera->GetRect().x);
		virtCamToDraw.y = (virtCam.y - camera->GetRect().y);
		virtCamToDraw.w = virtCam.w;
		virtCamToDraw.h = virtCam.h;
		SDL_SetRenderDrawColor(renderer, 0, 200, 10, 250);
		SDL_RenderDrawRect(renderer, &virtCamToDraw);
	}
}