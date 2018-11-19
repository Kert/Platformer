#ifndef _graphics_h_
#define _graphics_h_ 

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include "entities.h"
#include "globals.h"

#define WIDTH 768
#define HEIGHT 720

#define OVERVIEW_WIDTH 800
#define OVERVIEW_HEIGHT 800

#define RENDER_SCALE 3

int GraphicsSetup();
void ResetLevelGraphics();
bool GameRenderSetup();
bool MenuRenderSetup();
void BlitLevelTiles();
void GraphicsUpdate();
void GraphicsCleanup();
void GraphicsExit();

void Render(Entity &e);
void DrawHitbox(Entity &e);

void DrawFPS(Uint32 dt);
void ShowDebugInfo(Player &p);
void UpdateWindowMode();
void UpdateTransition();
void UpdateMenu();
void RenderLogo();
void ShowPauseOverlay();
void RenderMenu();
void RenderMenuItems(MENUS menu);
void RenderText(int x, int y, std::string text, TTF_Font *font, SDL_Color color);
void UpdateAnimation(Bullet &b);
void UpdateAnimation(Effect &e);
void UpdateAnimation(Player &p);
void UpdateAnimation(Creature &c);
void UpdateAnimation(Pickup &p);
void DrawFading();
void FlushWindow();
void UpdateWindow();
void UpdateTileAnimations();
SDL_Texture* GenerateLightningTexture(std::vector<SDL_Point> &points);
void ChangePlayerColor(PLAYER_BODY_PARTS bodyPart, SDL_Color color);
void SetupLevelGraphics(int map_width, int map_height);
void InitPlayerTexture();

class TextureManager
{
private:
	std::map<std::string, SDL_Texture*> textures;

public:
	void LoadTexture(std::string filename);
	void UnloadTexture(std::string filename);
	bool IsLoaded(std::string filename);
	SDL_Texture** TextureManager::GetTexture(std::string filename);
	void TextureManager::Clear();
};
#endif
