#include "levelspecific.h"

#include "camera.h"
#include "entities.h"
#include "level.h"
#include "utils.h"

extern Level* level;
RandomGenerator level_rg;
extern Camera* camera;
extern Lava_Floor *lava;

Timer bossLevelTimer{ 80 }, anvilTimer{ 600 }, fireTimer{ 1200 }, bossStartTimer{ 7000 };
bool bossBattleActivated = false;

void LevelLogic()
{
	bossStartTimer.Run();
	if(bossStartTimer.completed)
	{
		bossBattleActivated = true;
	}
	if(bossBattleActivated)
	{
		bossLevelTimer.Run();
		anvilTimer.Run();
		fireTimer.Run();
		if(anvilTimer.completed)
		{
			Creature* cr = new Creature("Anvil");
			cr->SetAI<AI_Anvil>();
			cr->AI->SetDistanceToReachX(16 * TILESIZE);
			cr->AI->SetDistanceToReachY(50 * TILESIZE);
			int xPos = level->CameraBounds[2].x + TILESIZE * level_rg.Generate(1, 15);
			int yPos = level->CameraBounds[2].y + level->CameraBounds[2].h - 17 * TILESIZE;
			cr->SetPos(xPos, yPos);

			Effect* eff = new Effect(EFFECT_ROCKETL_HIT);
			eff->SetPos(xPos - eff->hitbox->GetRect().w / 4, yPos + 3 * TILESIZE);
		}
		if(fireTimer.completed)
		{
			Creature* fr = new Creature("Jumpingfire");
			fr->SetAI<AI_Jumpingfire>();
			fr->AI->SetDistanceToReachX(16 * TILESIZE);
			fr->AI->SetDistanceToReachY(50 * TILESIZE);
			fr->SetPos(level->CameraBounds[2].x + TILESIZE * level_rg.Generate(1, 15), level->CameraBounds[2].y + level->CameraBounds[2].h + 2 * TILESIZE);
		}
		if(bossLevelTimer.completed)
		{
			lava->Activate();
			camera->Detach();
			if(level->CameraBounds[2].h > 15 * TILESIZE)
				level->CameraBounds[2].h--;
			SDL_Rect rect;
			rect.x = level->CameraBounds[2].x;
			rect.w = level->CameraBounds[2].w;
			rect.h = 15 * TILESIZE;
			rect.y = level->CameraBounds[2].y + level->CameraBounds[2].h - 15 * TILESIZE;

			camera->SetRect(rect);
		}
	}
}