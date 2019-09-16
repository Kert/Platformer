#ifndef _graphics_h_
#define _graphics_h_ 

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include "camera.h"
#include "entities.h"
#include "globals.h"

class TextureManager
{
private:
	std::map<std::string, SDL_Texture*> textures;

public:
	void LoadTexture(std::string filename);
	void UnloadTexture(std::string filename);
	bool IsLoaded(std::string filename);
	SDL_Texture** GetTexture(std::string filename);
	void Clear();
};

namespace Graphics
{
	int Init();
	void Update(double ticks);
	void Cleanup();

	Camera* GetCamera();
	void CreateCamera();
	void RemoveCamera();

	void Render(Entity &e);
	void DrawHitbox(Entity &e);

	void DrawFPS(long long dt);
	void ShowDebugInfo(Player &p);
	void UpdateDisplayMode();
	void RenderInterface();
	void RenderTransition();
	void RenderLogo();
	void RenderMenu();
	void RenderMenuItems(MENUS menu);
	void RenderText(int x, int y, std::string text, TTF_Font *font, SDL_Color color, TEXT_ALIGN align = TEXT_ALIGN_LEFT);
	void UpdateAnimation(Bullet &b);
	void UpdateAnimation(Effect &e);
	void UpdateAnimation(Player &p, double ticks);
	void UpdateAnimation(Creature &c);
	void UpdateAnimation(Pickup &p);
	void DrawFading();
	void WindowFlush();
	void WindowUpdate();
	void UpdateTileAnimations();
	SDL_Texture* GenerateLightningTexture(std::vector<SDL_Point> &points);
	void ChangePlayerColor(PLAYER_BODY_PARTS bodyPart, SDL_Color color);
	void InitPlayerTexture();
	int GetWindowNormalizedX(double val);
	int GetWindowNormalizedY(double val);
	int GetFullscreenMode();
	void SetFullscreenMode(int mode);
	SDL_DisplayMode GetDisplayMode();
	SDL_DisplayMode GetDisplayMode(int dispIndex, int modeIndex);
	std::map<int, std::vector<SDL_DisplayMode>> GetDisplayModes();
	int SetDisplayMode(SDL_DisplayMode mode);
	int GetDisplayIndex();
	void SetDisplayIndex(int index);
	int GetRefreshRate();
	int GetGameSceneWidth();
	int GetGameSceneHeight();
	SDL_Renderer* GetRenderer();
	int LoadLevelTexturesFromFile(std::string fileName);
	SDL_Texture* GetLevelTexture();
	TTF_Font* GetFont(FONTS font);
	void ScreenShake(double sec);
	void ScreenShakeUpdate(double ticks);
	void DrawLetterbox();
	SCALING_MODES GetScalingMode();
	void SetScalingMode(int mode);
}
#endif
