#include "physics.h"

#include <SDL.h>
#include "camera.h"
#include "gamelogic.h"
#include "level.h"
#include "sound.h"
#include "state.h"
#include "tiles.h"
#include "utils.h"

static double PHYSICS_SPEED_FACTOR = 1;
static double PHYSICS_SPEED = 0.001 * PHYSICS_SPEED_FACTOR;

extern std::vector<std::vector<std::vector<Tile*>>> tileLayers;

extern int GameState;

extern std::vector<Creature*> creatures;
extern std::vector<Machinery*> machinery;

extern Level *level;
extern Player *player;

std::pair<double, double> GetAngleSinCos(DynamicEntity &shooter)
{
	double angle;
	if(shooter.direction) angle = 0;
	else angle = 180;
	/*if(shooter.hasState(STATE_LOOKINGUP))
	{
		if(!angle)
			angle -= 45;
		else angle += 45;
	}*/
	angle = angle * M_PI / 180.0;
	return std::pair<double, double>(std::sin(angle), std::abs(std::cos(angle)));
}

void ProcessShot(WEAPONS weapon, Creature &shooter)
{
	const int offset_weapon_y = 1;

	if(weapon != WEAPONS::WEAPON_LIGHTNING)
	{
		// LEAK: bullets created here aren't properly disposed??
		Bullet *bullet = new Bullet(weapon, shooter);
		SDL_Rect rect = shooter.hitbox->GetRect();
		if(shooter.state->Is(CREATURE_STATES::DUCKING))
		{
			rect.y += 6;
			rect.x -= 6;
		}
		if(shooter.state->Is(CREATURE_STATES::HANGING))
		{
			rect.y += 2;
		}
		int adjustedX;
		if(shooter.direction == DIRECTION_RIGHT)
			adjustedX = rect.x + rect.w;
		else
			adjustedX = rect.x - bullet->hitbox->GetRect().w;

		int adjustedY;

		adjustedY = rect.y + (rect.h / 2) + offset_weapon_y;
		if(weapon == WEAPON_GROUNDSHOCKWAVE)
			adjustedY = rect.y + rect.h + offset_weapon_y;

		bullet->SetPos(adjustedX, adjustedY);
	}
	else
	{
		Lightning *lightning = new Lightning(shooter);
	}
	switch(weapon)
	{
		case WEAPONS::WEAPON_ROCKETL:
		{
			PlaySfx("rocketl_shot");
			break;
		}
		case WEAPONS::WEAPON_FLAME:
		{
			PlaySfx("flame_shot");
			break;
		}
		case WEAPONS::WEAPON_GRENADE:
		{
			//PlaySfx("rocketl_shot");
			break;
		}
	}
}

void DetectAndResolveEntityCollisions(Creature &p)
{
	Velocity vel;
	vel = p.GetVelocity();
	// Checking for collisions with entities
	bool foundCollision = false;

	p.pushedFrom.left = p.pushedFrom.right = p.pushedFrom.top = p.pushedFrom.bottom = false;
	for(auto &machy : machinery)
	{
		if(machy->type != MACHINERY_TYPES::MACHINERY_PLATFORM)
			continue;

		Platform *plat = (Platform*)machy;
		if(HasCollisionWithEntity(p, *plat))
		{
			if(p.IsAI())
			{
				p.SwitchDirection();
				p.Walk();
				break;
			}
			
			if(plat->solid)
			{
				double platCenterY = plat->GetY() - plat->hitbox->GetPRect().h / 2;
				double playerCenterY = p.yNew - p.hitbox->GetPRect().h / 2;
				double platCenterX = plat->GetX() + plat->hitbox->GetPRect().w / 2;
				double playerCenterX = p.xNew + p.hitbox->GetPRect().w / 2;
				if(abs(playerCenterY - platCenterY) > abs(playerCenterX - platCenterX))
				{
					if(playerCenterY <= platCenterY)
					{
						// moving out of platform to properly check for left/right collissions
						p.pushedFrom.bottom = true;
						p.yNew = plat->GetY() - plat->hitbox->GetPRect().h - 0.001;
						foundCollision = true;
					}
					else
					{
						p.pushedFrom.top = true;
						p.yNew = plat->GetY() + p.hitbox->GetPRect().h + 0.001;
					}
					if(HasCollisionWithEntity(p, *plat))
					{
						if(playerCenterX <= platCenterX)
						{
							p.pushedFrom.right = true;
							p.xNew = plat->GetX() - p.hitbox->GetPRect().w - 0.001;
						}							
						else
						{
							p.pushedFrom.left = true;
							p.xNew = plat->GetX() + plat->hitbox->GetPRect().w + 0.001;
						}							
					}
					if(foundCollision)
					{
						p.yNew = plat->GetY() - plat->hitbox->GetPRect().h;
						p.SetState(CREATURE_STATES::ONGROUND);
						p.onMachinery = true;
						p.AttachTo(plat);
					}
				}
				else
				{
					if(playerCenterX <= platCenterX)
					{
						p.pushedFrom.right = true;
						p.xNew = plat->GetX() - p.hitbox->GetPRect().w - 0.001;
					}						
					else
					{
						p.pushedFrom.left = true;
						p.xNew = plat->GetX() + plat->hitbox->GetPRect().w + 0.001;
					}						
					if(HasCollisionWithEntity(p, *plat))
					{
						if(playerCenterY <= platCenterY)
						{
							p.pushedFrom.bottom = true;
							p.yNew = plat->GetY() - plat->hitbox->GetPRect().h - 0.001;
						}							
						else
						{
							p.pushedFrom.top = true;
							p.yNew = plat->GetY() + p.hitbox->GetPRect().h + 0.001;
						}							
					}
				}
				continue;
			}
			if(plat->standable)
			{
				// standing on top
				foundCollision = true;
				if(abs(p.yNew - plat->hitbox->GetPRect().y) < 3)
				{
					if(p.GetVelocity().y < 0)
						continue;
					p.yNew = plat->hitbox->GetPRect().y + 1;
					p.SetState(CREATURE_STATES::ONGROUND);
					p.onMachinery = true;
					p.AttachTo(plat);
				}
				else
					foundCollision = false;
			}
			if(plat->hookable)
			{
				foundCollision = true;
				PrecisionRect hook = plat->hitbox->GetPRect();
				hook.h -= 4;
				hook.y += 4;
				PrecisionRect hand;
				hand.x = p.xNew + 2;
				hand.y = p.yNew - p.hitbox->GetRect().h;
				hand.w = hand.h = 2;

				if(HasIntersection(&hook, &hand))
				{
					p.nearhookplatform = true;
					if(p.GetVelocity().y > 0 && !p.lefthook)
					{
						p.SetState(CREATURE_STATES::HANGING);
						p.AttachTo(plat);
						vel.x = 0;
					}
				}
				else if(!HasIntersection(&plat->hitbox->GetPRect(), &hand))
					p.nearhookplatform = false;
			}
		}
	}

	if(!foundCollision)
	{
		if(p.attached)
			p.SetState(CREATURE_STATES::INAIR);
		p.onMachinery = false;
		p.attached = nullptr;
		p.nearhookplatform = false;
	}

	p.SetVelocity(vel.x, vel.y);
}


void CheckSpecialBehaviour(Creature &p) {
	SDL_Rect hangRect = p.hitbox->GetRect();
	hangRect.x = p.xNew;
	hangRect.y = p.yNew - hangRect.h;

	bool nearhook = false;
	int head = ConvertToTileCoord(hangRect.y, false);
	int minx = ConvertToTileCoord(hangRect.x, false);
	int maxx = ConvertToTileCoord(hangRect.x + hangRect.w, false);

	//if (minx == maxx)
	//	maxx++;
	// checking for hanging

	for(int z = minx; z <= maxx; z++)
	{
		PHYSICS_TYPES tileType = GetTileTypeAtTiledPos(z, head);
		if(tileType == PHYSICS_HOOK || tileType == PHYSICS_HOOK_PLATFORM)
		{
			SDL_Rect hook;
			if(tileType == PHYSICS_HOOK)
			{
				hook.x = z * TILESIZE + 6;
				hook.y = head * TILESIZE + 8;
				hook.w = hook.h = 10;
			}
			else if(tileType == PHYSICS_HOOK_PLATFORM)
			{
				hook.x = z * TILESIZE;
				hook.y = head * TILESIZE;
				hook.w = hook.h = TILESIZE;
			}
			SDL_Rect hand;
			hand.x = p.xNew + 4;
			hand.y = p.yNew - p.hitbox->GetRect().h + 2;
			hand.w = hand.h = 3;

			if(SDL_HasIntersection(&hook, &hand))
			{
				nearhook = true;

				if(p.GetVelocity().y > 0 && !p.lefthook)
				{
					p.SetState(CREATURE_STATES::HANGING);
					break;
				}
			}
		}
	}

	if(p.lefthook && (!nearhook && !p.nearhookplatform))
		p.lefthook = false;
}

void ApplyPhysics(Creature &p, Uint32 deltaTicks)
{
	if(p.REMOVE_ME)
		return;

	ApplyForces(p, deltaTicks);
	if(IsInDeathZone(p) && !p.ignoreGravity)
	{
		if(&p == player)
			GameOver(GAME_OVER_REASON_DIED);
		else
			p.REMOVE_ME = true;
		return;
	}

	if(!p.ignoreWorld)
	{
		DetectAndResolveMapCollisions(p);
		if(!p.IsAI())
			DetectAndResolveEntityCollisions(p);
		CheckSpecialBehaviour(p);
		if(p.attached)
		{
			// TODO: move to a separate func
			p.attX = p.xNew - p.attached->GetX();
			p.attY = p.yNew - p.attached->GetY() + p.attached->hitbox->GetPRect().h;
		}
	}
	p.SetPos(p.xNew, p.yNew);
	

	if(GameState != STATE_GAME) return; // we hit the exit block, don't process further

	//DetectAndResolveEntityCollisions(p);
	UpdateStatus(p, deltaTicks);
}

bool IsInDeathZone(Creature &c)
{
	for(auto i : level->deathZones)
	{
		if(SDL_HasIntersection(&c.hitbox->GetRect(), &i))
			return true;
	}
	return false;
}

bool IsOnIce(Creature &c)
{
	PrecisionRect ppr = c.hitbox->GetPRect();
	int minx = ConvertToTileCoord(c.xNew, false);
	int maxx = ConvertToTileCoord(c.xNew + ppr.w, true);
	int feet = ConvertToTileCoord(c.yNew + 1, false);

	// Bugfix for possible skipping of the next loop
	if(minx == maxx)
		maxx = minx + 1;

	for(int i = minx; i < maxx; i++)
	{
		if(feet < 1) break;
		switch(GetTileTypeAtTiledPos(i, feet))
		{
			case PHYSICS_ICEBLOCK: case PHYSICS_ICE:
			{
				//PrintLog(LOG_SUPERDEBUG, "I'm ON ICE!!!!!!");
				return true;
			}
		}
	}
	return false;
}

bool IsOnPlatform(Creature &c)
{
	PrecisionRect ppr = c.hitbox->GetPRect();
	int minx = ConvertToTileCoord(c.xNew, false);
	int maxx = ConvertToTileCoord(c.xNew + ppr.w, true);
	int feet = ConvertToTileCoord(c.yNew + 1, false);

	// Bugfix for possible skipping of the next loop
	if(minx == maxx)
		maxx = minx + 1;

	for(int i = minx; i < maxx; i++)
	{
		if(feet < 1) break;
		switch(GetTileTypeAtTiledPos(i, feet))
		{
			case PHYSICS_PLATFORM: case PHYSICS_HOOK_PLATFORM:
			{
				//PrintLog(LOG_SUPERDEBUG, "I'm ON ICE!!!!!!");
				return true;
			}
		}
	}
	if(c.onMachinery)
		return true;
	return false;
}

bool IsInRain(DynamicEntity &c)
{
	PrecisionRect ppr = c.hitbox->GetPRect();
	double x, y;
	c.GetPos(x, y);
	int minx = ConvertToTileCoord(x, false);
	int maxx = ConvertToTileCoord(ppr.x + ppr.w, true);
	int head = ConvertToTileCoord(ppr.y, false);
	int feet = ConvertToTileCoord(y, true);

	// Bugfix for possible skipping of the next loops
	if(minx == maxx)
		maxx = minx + 1;
	if(feet == head)
		feet = head + 1;
	for(int i = minx; i < maxx; i++)
	{
		for(int j = head; j < feet; j++)
		{
			switch(GetTileTypeAtTiledPos(i, j))
			{
				case PHYSICS_RAIN:
				{
					PrintLog(LOG_SUPERDEBUG, "I'm RAINY!!!!!!");
					return true;
				}
			}
		}
	}
	return false;
}

bool IsInWater(DynamicEntity &c)
{
	PrecisionRect ppr = c.hitbox->GetPRect();
	double x, y;
	c.GetPos(x, y);
	int minx = ConvertToTileCoord(x, false);
	int maxx = ConvertToTileCoord(ppr.x + ppr.w, true);
	int head = ConvertToTileCoord(ppr.y, false);
	int feet = ConvertToTileCoord(y, true);

	// Bugfix for possible skipping of the next loops
	if(minx == maxx)
		maxx = minx + 1;
	if(feet == head)
		feet = head + 1;
	for(int i = minx; i < maxx; i++)
	{
		for(int j = head; j < feet; j++)
		{
			switch(GetTileTypeAtTiledPos(i, j))
			{
				case PHYSICS_WATER:
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool IsSolid(PHYSICS_TYPES type)
{
	return type == PHYSICS_BLOCK || type == PHYSICS_ICEBLOCK || type == PHYSICS_ICE;
}

bool HasCeilingRightAbove(DynamicEntity &c)
{
	PrecisionRect ppr = c.hitbox->GetPRect();
	double x, y;
	c.GetPos(x, y);
	int minx = ConvertToTileCoord(x, false);
	int maxx = ConvertToTileCoord(ppr.x + ppr.w, true);
	int head = ConvertToTileCoord(ppr.y, false);

	// Bugfix for possible skipping of the next loops
	if(minx == maxx)
		maxx = minx + 1;
	for(int i = minx; i < maxx; i++)
	{
		if(head - 1 < 0) return true;
		if(IsSolid(GetTileTypeAtTiledPos(i, head - 1)))
		{
			PrintLog(LOG_SUPERDEBUG, "I'm RIGHT BELOW CEILING!!!");
			return true;
		}
	}
	return false;
}

void ApplyForces(Creature &p, Uint32 deltaTicks)
{
	/*acceleration = force(time, position, velocity) / mass;
	time += timestep;
	position += timestep * (velocity + timestep * acceleration / 2);
	velocity += timestep * acceleration;
	newAcceleration = force(time, position, velocity) / mass;
	velocity += timestep * (newAcceleration - acceleration) / 2;*/

	const double JUMP_VELOCITY = -155;
	const double GROUND_FRICTION = 4;
	const double AIR_FRICTION = 10;
	const double ICE_SLIDING_FRICTION = 0.4;
	const double ICE_WALKING_FRICTION = 1.5;
	const double WATER_FRICTION_MULTIPLIER = 1.5;
	const double WATER_MAX_VELOCITY_X_MULTIPLIER = 0.5;
	const double WATER_MAX_VELOCITY_Y_MULTIPLIER = 0.5;

	Velocity vel; // Resulting velocity
	vel = p.GetVelocity();

	if((p.state->Is(CREATURE_STATES::ONGROUND) && p.status != STATUS_STUN) || p.state->Is(CREATURE_STATES::SLIDING) || p.state->Is(CREATURE_STATES::HANGING))
		vel.y = 0;

	if(!p.ignoreGravity)
	{
		p.accel.y = 5 * p.gravityMultiplier;
	}

	if(p.state->Is(CREATURE_STATES::HANGING))
	{
		p.accel.y = 0;
		p.accel.x = 0;
	}

	if(p.onMachinery)
		p.accel.y = 0;

	if(p.jumptime > 0)
		vel.y = JUMP_VELOCITY;

	p.SetVelocity(vel);

	vel = p.GetVelocity();

	vel.y += p.accel.y;
	if(!p.state->Is(CREATURE_STATES::ONGROUND) && !p.ignoreWorld)
		p.accel.x *= 99999;
	vel.x += p.accel.x * deltaTicks;

	double friction = GROUND_FRICTION;
	if(!p.state->Is(CREATURE_STATES::ONGROUND))
		friction = AIR_FRICTION;

	if(IsOnIce(p))
	{
		if(p.state->Is(CREATURE_STATES::SLIDING))
			friction = ICE_SLIDING_FRICTION;
		else
			friction = ICE_WALKING_FRICTION;
	}

	if(IsInWater(p))
		friction *= WATER_FRICTION_MULTIPLIER;

	if(p.ignoreWorld)
		friction = 0;

	if(!(HasCeilingRightAbove(p) && p.state->Is(CREATURE_STATES::SLIDING)))
	{
		if(vel.x > 0)
		{
			vel.x -= friction;
			if(vel.x < 0)
				vel.x = 0;
		}
		if(vel.x < 0)
		{
			vel.x += friction;
			if(vel.x > 0)
				vel.x = 0;
		}
	}

	// Limiting forces
	double maxVelX, maxVelY;
	maxVelY = abs(p.term_vel);
	maxVelX = abs(p.move_vel);
	if(IsInWater(p))
	{
		maxVelX *= WATER_MAX_VELOCITY_X_MULTIPLIER;
		maxVelY *= WATER_MAX_VELOCITY_Y_MULTIPLIER;
	}
	if(abs(vel.x) > maxVelX)
		vel.x = maxVelX * (vel.x < 0 ? -1 : 1);
	if(abs(vel.y) > maxVelY)
		vel.y = maxVelY * (vel.y < 0 ? -1 : 1);

	// Saving new velocity
	p.SetVelocity(vel);

	// Modifying position
	vel = p.GetVelocity();

	p.xNew = p.GetX();
	p.yNew = p.GetY();
	if(p.attached)
	{
		p.xNew = p.attached->hitbox->GetPRect().x + p.attX;
		p.yNew = p.attached->hitbox->GetPRect().y + p.attY;
	}

	p.xNew += vel.x * (deltaTicks * PHYSICS_SPEED);
	p.yNew += vel.y * (deltaTicks * PHYSICS_SPEED);
}

extern Camera *camera;

void ApplyPhysics(Machinery &d, Uint32 deltaTicks)
{
	double x, y;
	Velocity vel;
	vel = d.GetVelocity();
	d.GetPos(x, y);
	
	// Doors
	if(d.type == MACHINERY_TYPES::MACHINERY_DOOR)
	{
		y += vel.y * (deltaTicks * PHYSICS_SPEED);

		// BROKE DOORS
		if(y < d.default_pos.y - d.default_pos.h)
		{
			y = d.default_pos.y - d.default_pos.h;
			vel.y = 0;
		}
		else if(y > d.default_pos.y)
		{
			y = d.default_pos.y;
			vel.y = 0;
		}
	}
	if(d.type == MACHINERY_TYPES::MACHINERY_LAVAFLOOR)
	{
		if(d.attachToScreen)
		{
			SDL_Rect rect;
			rect = camera->GetRect();
			x = rect.x + d.attachScreenX;
			y = rect.y + d.attachScreenY;
		}
	}
	if(d.type == MACHINERY_TYPES::MACHINERY_PLATFORM)
	{
		Platform *plat = (Platform*)&d;
		// Up-down platform
		if(plat->default_pos.y != plat->another_pos.y)
		{
			// Using velocity to determine new pos
			y += vel.y * (deltaTicks * PHYSICS_SPEED);
			// Limiting speed
			if(abs(vel.y) > plat->speed)
				vel.y = plat->speed * (vel.y < 0 ? -1 : 1);
			// Do stuff when approaching top end point
			if(abs(y - (plat->default_pos.y)) < TILESIZE)
				vel.y += plat->deaccel.y * (deltaTicks * PHYSICS_SPEED);
			// Platform reached top end point
			if(y < plat->default_pos.y)
			{
				// Let it go the other direction
				vel.y = plat->minspeed;
				y = plat->default_pos.y + 1;
			}
			else
			{
				// Do stuff when approaching bottom end point
				if(abs(y - plat->another_pos.y) < TILESIZE)
					vel.y -= plat->deaccel.y * (deltaTicks * PHYSICS_SPEED);
				// Platform reached bottom end point
				if(y > plat->another_pos.y)
				{
					// Let it go the other direction
					vel.y = -plat->minspeed;
					y = plat->another_pos.y - 1;
				}
			}
			// Don't stop me now
			if(abs(vel.y) < plat->minspeed)
				vel.y = plat->minspeed * (vel.y > 0 ? 1 : -1);
		}
		else // Left-right platform
		{
			// Using velocity to determine new pos
			x += vel.x * (deltaTicks * PHYSICS_SPEED);
			// Limiting speed
			if(abs(vel.x) > plat->speed)
				vel.x = plat->speed * (vel.x < 0 ? -1 : 1);
			// Do stuff when approaching left end point
			if(abs(x - (plat->default_pos.x)) < TILESIZE)
				vel.x += plat->deaccel.x * (deltaTicks * PHYSICS_SPEED);
			// Platform reached left end point
			if(x < plat->default_pos.x)
			{
				// Let it go the other direction
				vel.x = plat->minspeed;
				x = plat->default_pos.x + 1;
			}
			else
			{
				// Do stuff when approaching bottom end point
				if(abs(x - plat->another_pos.x) < TILESIZE)
					vel.x -= plat->deaccel.x * (deltaTicks * PHYSICS_SPEED);
				// Platform reached right end point
				if(x > plat->another_pos.x)
				{
					vel.x = -plat->minspeed;
					x = plat->another_pos.x - 1;
				}
			}
			// Don't stop me now
			if(abs(vel.x) < plat->minspeed)
				vel.x = plat->minspeed * (vel.x > 0 ? 1 : -1);
		}
	}

	d.SetVelocity(vel);
	d.SetPos(x, y);
}

bool ApplyPhysics(Bullet &b, Uint32 deltaTicks)
{
	const double IN_RAIN_FIREBALL_DECAY_MULTIPLIER = 3;

	double x, y;
	Velocity vel;
	bool complete = false;
	vel = b.GetVelocity();
	Accel accel = b.accel;
	b.GetPos(x, y);

	y += vel.y * (deltaTicks * PHYSICS_SPEED);
	x += vel.x * (deltaTicks * PHYSICS_SPEED);
	vel.y += accel.y;
	vel.x += accel.x;

	b.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
	if(IsInRain(b) && b.origin == WEAPON_FIREBALL)
		b.statusTimer -= (int)(IN_RAIN_FIREBALL_DECAY_MULTIPLIER * deltaTicks * PHYSICS_SPEED_FACTOR);
	else
		b.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);

	if(b.statusTimer <= 0)
	{
		b.statusTimer = 0;
		if(b.origin == WEAPON_GRENADE || b.origin == WEAPON_LIGHTNING)
		{
			complete = true;
		}
		else
		{
			b.Remove();
			return false;
		}
	}

	std::pair<Creature*, Machinery*> wasHit;
	wasHit = CheckForCollision(&b);
	// do not allow enemies to damage other enemies
	if(b.owner == player || b.owner != player && wasHit.first == player)
	{
		if(wasHit.first != nullptr && wasHit.first != b.owner)
		{
			wasHit.first->ProcessBulletHit(&b);
			complete = true;
		}
	}
	if(wasHit.second != nullptr && wasHit.second->solid)
	{
		if(wasHit.second->destructable && b.origin == WEAPON_ROCKETL) wasHit.second->~Machinery();
		complete = true;
	}

	int tileX = (int)(ceil((double)(x + (b.hitbox->GetRect().w * b.direction)) / (TILESIZE)) - 0.5);
	int tileY = ConvertToTileCoord(y - b.hitbox->GetRect().h / 2, 0);

	PHYSICS_TYPES type = GetTileTypeAtTiledPos(tileX, tileY);
	if(tileX >= level->width_in_tiles || tileX < 0 || tileY >= level->height_in_tiles || tileY < 0)
		complete = true;
	else if(IsSolid(type))
	{
		PrintLog(LOG_SUPERDEBUG, "I hit block at %d %d", tileX, tileY);

		if(b.origin == WEAPON_GRENADE)
		{
			// find out where we bounced from and reverse that
			int checktileX = (int)(ceil(tileX + (b.direction ? -1 : 1) - 0.5));
			int checktileY = (int)(ceil(tileY + (b.GetVelocity().y < 0 ? 1 : -1)));

			if(!IsSolid(GetTileTypeAtTiledPos(checktileX, tileY)))
			{
				b.SwitchDirection();
				x += b.direction ? -1 : 1;
				vel.x *= -0.5;
			}
			if(!IsSolid(GetTileTypeAtTiledPos(tileX, checktileY)))
			{
				y += b.GetVelocity().y < 0 ? 1 : -1;
				vel.y *= -0.5;
			}
		}
		else if(b.origin == WEAPON_FIREBALL)
		{
			if(type == PHYSICS_ICEBLOCK)
			{
				// TODO: Optimize this?
				for(auto &layer : tileLayers)
				{
					if(layer[tileX][tileY] != nullptr)
					{
						if(layer[tileX][tileY]->type == PHYSICS_ICEBLOCK)
						{
							delete layer[tileX][tileY];							
						}
					}
				}
			}
		}
		else
		{
			complete = true;
		}
	}

	if(x >= level->width_in_pix || y >= level->height_in_pix || x < 0 || y < 0)
	{
		complete = true;
	}

	if(complete && b.origin != WEAPON_FLAME)
	{
		Effect *effect;
		switch(b.origin)
		{
			case WEAPON_ROCKETL:
			case WEAPON_GRENADE:
			case WEAPON_BOMBDROP:
				effect = new Effect(EFFECT_ROCKETL_HIT);
				effect->SetPos(x - 11, y + 11);
				PlaySfx("rocketl_explode");
				b.Remove();
				return false;
			case WEAPON_LIGHTNING:
				//b.Remove();
				return false;
		}
	}

	b.SetVelocity(vel);
	b.SetPos(x, y);
	return true;
}

bool ApplyPhysics(Lightning &l, Uint32 deltaTicks)
{
	l.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
	if(IsInRain(l))
		l.statusTimer -= (int)(3 * deltaTicks * PHYSICS_SPEED_FACTOR);
	else
		l.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);

	if(l.statusTimer <= 0)
	{
		l.statusTimer = 0;
		l.Remove();
		return false;
	}

	for(auto &j : creatures)
	{
		if(j == nullptr)
			continue;
		if(l.hitbox->HasCollision(j->hitbox))
			j->TakeDamage(100);
	}

	return true;
}


void ResolveBottom(Creature &p)
{
	PrecisionRect ppr = p.hitbox->GetPRect();
	int minx = ConvertToTileCoord(p.GetX(), false);
	int maxx = ConvertToTileCoord(p.GetX() + ppr.w, true);
	//int maxx = ceil((ppr.x + ppr.w) / (double)TILESIZE);
	int feet = ConvertToTileCoord(p.yNew, false);

	// Bugfix for possible skipping of the next loop
	if(minx == maxx)
		maxx = minx + 1;

	// Bottom
	bool collisionFound = false;
	SDL_Rect tileBottom;
	for(int i = minx; i < maxx; i++)
	{
		if(collisionFound || feet < 1) break;

		tileBottom = GetTileRect(i, feet);
		PHYSICS_TYPES type = GetTileTypeAtTiledPos(i, feet);
		if(IsSolid(type))
		{
			//PrintLog(LOG_IMPORTANT, "Intersecting block bottom at %d. Returning back to y = %lf", tileBottom.y, p.yNew);
			collisionFound = true;
			break;
		}
		switch(type)
		{
			case PHYSICS_PLATFORM: case PHYSICS_HOOK_PLATFORM:
			{
				//PrintLog(LOG_SUPERDEBUG, "Tile intersection: Platform");
				if(p.GetVelocity().y >= 0 && (p.yNew - tileBottom.y) < 2)
				{
					//PrintLog(LOG_SUPERDEBUG, "Intersecting platform at %d by %d. Returning back to y = %d", pr.y, result.h, y);
					collisionFound = true;
					break;
				}
			}
		}
	}

	if(!collisionFound)
	{
		if(!p.state->Is(CREATURE_STATES::JUMPING) && !p.state->Is(CREATURE_STATES::HANGING) && !p.onMachinery)
			p.SetState(CREATURE_STATES::INAIR);
	}
	else
	{
		// because we need to be out of the ground for next left/right colission check
		p.yNew = tileBottom.y - 0.001;
		if(!p.state->Is(CREATURE_STATES::SLIDING))
			p.SetState(CREATURE_STATES::ONGROUND);

		p.SetVelocity(p.GetVelocity().x, 0);
		if(p.pushedFrom.top)
		{
			p.Crush();
		}
	}
}

void ResolveTop(Creature &p)
{
	PrecisionRect ppr = p.hitbox->GetPRect();

	int minx = ConvertToTileCoord(p.GetX(), false);
	int maxx = ConvertToTileCoord(p.GetX() + ppr.w, true);
	int head = ConvertToTileCoord(p.yNew - ppr.h, false);

	// Bugfix for possible skipping of the next loop
	if(minx == maxx)
		maxx = minx + 1;

	bool collisionFound = false;
	for(int i = minx; i < maxx; i++)
	{
		PHYSICS_TYPES type = GetTileTypeAtTiledPos(i, head);
		if(IsSolid(type))
		{
			p.yNew = p.GetY(); // REVERT
			collisionFound = true;
			//PrintLog(LOG_SUPERDEBUG, "Hitting ceiling");
			break;
		}
	}
	if(collisionFound)
	{
		if(p.pushedFrom.bottom)
		{
			p.Crush();
		}
	}
}

void ResolveRight(Creature &p)
{
	Velocity vel; // Player velocity
	vel = p.GetVelocity();

	PrecisionRect ppr = p.hitbox->GetPRect();
	// Converting to coordinates in tiles array
	int maxx = ConvertToTileCoord(p.xNew + ppr.w, false);

	int feet = ConvertToTileCoord(p.yNew, false);
	int head = ConvertToTileCoord(p.yNew - ppr.h, false);

	bool break_flag = false;
	bool collisionFound = false;
	for(int j = head; j <= feet; j++)
	{
		if(break_flag) break;
		PHYSICS_TYPES type = GetTileTypeAtTiledPos(maxx, j);
		if(IsSolid(type))
		{
			collisionFound = true;
			p.accel.x = 0;
			vel.x = 0;			
			//PrintLog(LOG_SUPERDEBUG, "Intersecting wall right at %d. Returning back to x = %d", pr.x, x);	
			break;
		}
		switch(type)
		{
			case PHYSICS_EXITBLOCK:
			{
				if(!p.IsAI()) // only the player
				{
					OnLevelExit(); // EXIT LEVEL
					return;
				}
				break;
			}
		}
	}

	// Modify velocity after all collision resolutions
	p.SetVelocity(vel.x, vel.y);

	if(collisionFound)
	{
		p.xNew = maxx * TILESIZE - ppr.w - 0.001;
		if(p.pushedFrom.left)
		{
			p.Crush();
		}
	}
}

void ResolveLeft(Creature &p)
{
	Velocity vel; // Player velocity
	vel = p.GetVelocity();

	PrecisionRect ppr = p.hitbox->GetPRect();
	// Converting to coordinates in tiles array
	int minx = ConvertToTileCoord(p.xNew, false);
	int feet = ConvertToTileCoord(p.yNew, false);
	int head = ConvertToTileCoord(p.yNew - ppr.h, false);

	// Left
	bool break_flag = false;
	bool collisionFound = false;
	for(int j = head; j <= feet; j++)
	{
		if(break_flag) break;
		PHYSICS_TYPES type = GetTileTypeAtTiledPos(minx, j);
		if(IsSolid(type))
		{
			//PrintLog(LOG_INFO, "Intersecting wall left at %lf. Returning back to x = %lf", ppr.x, p.xNew);
			p.accel.x = 0;
			vel.x = 0;
			collisionFound = true;
			break;
		}
		switch(type)
		{
			case PHYSICS_EXITBLOCK:
			{
				if(!p.IsAI()) // only the player
				{
					OnLevelExit(); // EXIT LEVEL
					return;
				}
				break;
			}
		}
	}

	// Modify velocity after all collision resolutions
	p.SetVelocity(vel.x, vel.y);

	if(collisionFound)
	{
		p.xNew = (minx + 1) * TILESIZE + 0.001;
		if(p.pushedFrom.right)
		{
			p.Crush();
		}
	}
}

void DetectAndResolveMapCollisions(Creature &p)
{
	const double OOB_EXTENT = 100;

	ResolveBottom(p);
	ResolveTop(p);
	ResolveRight(p);
	ResolveLeft(p);

	//Allow out of bounds coords, but not much
	if(p.GetX() < -OOB_EXTENT)
		p.SetX(-OOB_EXTENT);
	else if(p.GetX() + p.hitbox->GetRect().w > level->width_in_pix + OOB_EXTENT)
		p.SetX(level->width_in_pix - p.hitbox->GetRect().w + OOB_EXTENT);
	if(p.GetY() - p.hitbox->GetRect().h < -OOB_EXTENT)
		p.SetY(p.hitbox->GetRect().h - OOB_EXTENT);
	else if(p.GetY() > level->height_in_pix + OOB_EXTENT)
		p.SetY(level->height_in_pix + OOB_EXTENT);
}

bool HasCollisionWithEntity(Creature &p, Machinery &m)
{
	bool foundSpecialInteraction = false;
	PrecisionRect prect = p.hitbox->GetPRect();
	prect.x = p.xNew;
	prect.y = p.yNew - prect.h;
	PrecisionRect *dyrect = &m.hitbox->GetPRect();
	if(HasIntersection(&prect, dyrect))
	{
		if(m.type == MACHINERY_TYPES::MACHINERY_BUTTON)
		{
			Button *btn = (Button*)&m;
			p.interactTarget = btn->pairID;
			foundSpecialInteraction = true;
		}
		return true;
		//	if (result.h > 0 && result.h < result.w && !dy->automatic && !dy->enabled && dy->GetVelocity().y > 0) 
		//		dy->Activate(); // Door is closing on something, open it back up
		//	return true;
	}
	if(!foundSpecialInteraction) p.interactTarget = -1;
	return false;
}

void UpdateStatus(Creature &p, Uint32 deltaTicks)
{
	if(p.REMOVE_ME)
		return;

	if(p.status != STATUS_NORMAL)
	{
		if(p.statusTimer > 0)
		{
			p.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
			if(p.statusTimer <= 0) // time to switch to another status!
			{
				p.statusTimer = 0;
				switch(p.status)
				{
					case STATUS_STUN:
						p.SetInvulnerability(1 * 1000);
						p.accel.x = 0;
						if(p.health <= 0)
							p.status = STATUS_DYING;
						break;
					case STATUS_INVULN:
						p.status = STATUS_NORMAL;
						break;
					case STATUS_DYING:
						//p.REMOVE_ME = true;
						//p.Remove(); //<-- todo: I think this can be implemented now
						return;
					default:
						break;
				}
			}
		}
	}

	// Player timers

	if(p.jumptime > 0)
	{
		p.jumptime -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
		if(p.jumptime <= 0)
		{
			p.jumptime = 0;
		}
	}

	if(p.shottime > 0)
	{
		p.shottime -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
		if(p.shottime <= 0)
		{
			p.shottime = 0;
			p.shotLocked = false;
		}
	}

	if(p.charge_time > 0)
	{
		p.charge_time -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
		if(p.charge_time <= 0)
		{
			p.charge_time = 0;
		}
	}

	for(auto j = p.hitFrom.begin(); j != p.hitFrom.end();)
	{
		j->immunity--;
		if(j->immunity <= 0)
			j = p.hitFrom.erase(j);
		else
			++j;
	}
}

bool UpdateStatus(Effect &e, Uint32 deltaTicks) // used for static entity death processing for now
{
	e.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
	if(e.statusTimer <= 0)
	{
		e.statusTimer = 0;
		e.Remove();
		return false;
	}
	return true;
}

bool UpdateStatus(Pickup &e, Uint32 deltaTicks) // used for static entity death processing for now
{
	e.statusTimer -= (int)(deltaTicks * PHYSICS_SPEED_FACTOR);
	if(e.statusTimer <= 0)
	{
		e.statusTimer = 0;
		e.Remove();
		return false;
	}
	return true;
}

// Returns pointers to entities that were hit
std::pair<Creature*, Machinery*> CheckForCollision(Bullet *entity)
{
	Creature* c = NULL;
	Machinery* m = NULL;
	for(auto &j : creatures)
	{
		if(j == nullptr)
			continue;
		if(entity->hitbox->HasCollision(j->hitbox))
		{
			c = j;
			break;
		}
	}
	if(entity->hitbox->HasCollision(player->hitbox))
		c = player;
	for(auto &ma : machinery)
	{
		if(entity->hitbox->HasCollision(ma->hitbox))
		{
			m = ma;
			break;
		}
	}
	return std::pair<Creature*, Machinery*>(c, m);
}

void OnHitboxCollision(Creature &c, Creature &e, Uint32 deltaTicks)
{
	if(c.status == STATUS_NORMAL && e.status != STATUS_DYING)
	{
		c.SetState(CREATURE_STATES::INAIR);
		PlaySfx("player_hit");
		c.TakeDamage(25);
		ApplyKnockback(c, e);
	}
}

// todo: why do we need creature c?
void OnHitboxCollision(Creature &c, Pickup &p, Uint32 deltaTicks)
{
	if(p.status != STATUS_DYING) // probably a collectable!
	{
		p.OnPickup();
	}
}

void ApplyKnockback(Creature &p, Creature &e)
{
	const int STUN_TIME = 200;
	const double KNOCKBACK_VELOCITY_X = 300;
	const double KNOCKBACK_VELOCITY_Y = -100;
	const double KNOCKBACK_ACCEL_X = -4;

	p.SetStun(STUN_TIME);
	int knockbackDirection = e.GetX() > p.GetX() ? 1 : -1;
	p.SetVelocity(-KNOCKBACK_VELOCITY_X * knockbackDirection, KNOCKBACK_VELOCITY_Y);
	p.accel.x = KNOCKBACK_ACCEL_X * knockbackDirection;
}