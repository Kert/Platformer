#include "entities.h"
#include <algorithm>
#include "INIReader.h"
#include "SDL_mixer.h"
#include "camera.h"
#include "graphics.h"
#include "interface.h"
#include "physics.h"
#include "sound.h"
#include "state.h"
#include "tiles.h"
#include "utils.h"

extern SDL_Texture *player_texture;
extern SDL_Surface *lightningSegment;
extern SDL_Renderer *renderer;

extern RandomGenerator *ai_rg;

std::vector<Bullet*> bullets;
std::vector<Lightning*> lightnings;
std::vector<Creature*> creatures;
std::vector<Machinery*> machinery;
std::vector<Pickup*> pickups;
std::vector<Effect*> effects;

int doorPairs = 0;
extern Player *player;
extern Camera *camera;
extern TextureManager textureManager;

std::map<std::string, CreatureData> creatureData;
std::map<std::string, CreatureGraphicsData> creatureGraphicsData;

void ReadCreatureData()
{
	std::vector<std::string> fileList;
	GetFolderFileList("assets/data/creatures", fileList);
	for(auto i : fileList)
	{
		INIReader reader("assets/data/creatures/" + i);

		if(reader.ParseError() < 0)
		{
			PrintLog(LOG_IMPORTANT, "Can't load %s", i);
			return;
		}

		CreatureData cr;
		std::string name = reader.Get("Properties", "Name", "NULL");
		cr.ignoreWorld = reader.GetBoolean("Properties", "IgnoreWorld", false);
		cr.ignoreGravity = reader.GetBoolean("Properties", "IgnoreGravity", false);
		cr.gravityMultiplier = reader.GetReal("Properties", "GravityMultiplier", 1);
		cr.health = reader.GetInteger("Properties", "Health", 100);
		cr.term_vel = reader.GetReal("Properties", "TerminalVelocity", 400);
		cr.move_vel = reader.GetReal("Properties", "MoveVelocity", 100);
		cr.graphicsName = reader.Get("Properties", "Graphics", "dummy.ini");
		cr.weapon = (WEAPONS)reader.GetInteger("Properties", "Weapon", WEAPON_ROCKETL);
		cr.blinkDamaged = reader.GetBoolean("Properties", "BlinkDamaged", true);
		creatureData[name] = cr;
	}
	fileList.clear();

	GetFolderFileList("assets/data/graphics", fileList);
	for(auto j : fileList)
	{
		std::string fileName = "assets/data/graphics/" + j;
		INIReader rea(fileName);

		if(rea.ParseError() < 0)
		{
			PrintLog(LOG_IMPORTANT, "Can't load config.ini");
			return;
		}

		CreatureGraphicsData cr;

		std::string textureName = rea.Get("Sprite", "TextureName", "dummy.png");
		cr.sprite.SetSpriteTexture(textureManager.GetTexture(textureName));

		SDL_Rect rect;
		rect.x = rea.GetInteger("Sprite", "X", 0);
		rect.y = rea.GetInteger("Sprite", "Y", 0);
		rect.w = rea.GetInteger("Sprite", "Width", 0);
		rect.h = rea.GetInteger("Sprite", "Height", 0);
		cr.sprite.SetSpriteRect(rect);

		int offX = rea.GetInteger("Sprite", "OffsetX", 0);
		int offY = rea.GetInteger("Sprite", "OffsetY", 0);
		cr.sprite.SetSpriteOffset(offX, offY);

		rect.x = rea.GetInteger("Hitbox", "X", 0);
		rect.y = rea.GetInteger("Hitbox", "Y", 0);
		rect.w = rea.GetInteger("Hitbox", "Width", 0);
		rect.h = rea.GetInteger("Hitbox", "Height", 0);
		cr.hitbox.SetRect(rect);

		std::map<std::string, ANIMATION_TYPE> animTable = {
			{"Running", ANIMATION_RUNNING},
			{"Standing", ANIMATION_STANDING},
			{"Jumping", ANIMATION_JUMPING},
			{"Falling", ANIMATION_FALLING}
		};

		for(auto i : animTable)
		{
			std::string animString = rea.Get("Animations", i.first, "");
			if(animString != "")
			{
				std::vector<std::string> tokens;
				tokenize(animString, tokens, ",");
				int animOffX, animOffY, animNumFrames, animInterval, animFps, animLoopType;
				if(tokens.size() == 6)
				{
					animOffX = atoi(tokens[0].c_str());
					animOffY = atoi(tokens[1].c_str());
					animNumFrames = atoi(tokens[2].c_str());
					animInterval = atoi(tokens[3].c_str());
					animFps = atoi(tokens[4].c_str());
					animLoopType = atoi(tokens[5].c_str());
				}
				cr.sprite.AddAnimation(i.second, animOffX, animOffY, animNumFrames, animInterval, animFps, (ANIM_LOOP_TYPES)animLoopType);
			}
		}
		creatureGraphicsData[fileName] = cr;
	}
}


Player::Player()
{
	move_vel = 110;
	climb_vel = 95;
	jumptime = 0;
	term_vel = 240;
	health = 100;
	direction = DIRECTION_RIGHT;
	status = 0;
	statusTimer = 0;
	shottime = 0;
	externSpeed.x = 0;
	externSpeed.y = 0;

	shotLocked = false;
	charging = false;
	onMachinery = false;
	
	ResetWeapons();

	ammo[WEAPON_FIREBALL] = 3;
	//Initialize the velocity
	SetVelocity(0, 0);
	
	hitbox = new Hitbox(0, 0, 30, 10);
	sprite = new Sprite(&player_texture, 0, 0, 34, 32);
	sprite->SetSpriteOffset(-11, -32 + 1);
	sprite->AddAnimation(ANIMATION_RUNNING, 128, 0, 4, 32, 150, ANIM_LOOP_TYPES::LOOP_NORMAL);
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_CLIMBING, 96, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_JUMPING, 32, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_FALLING, 64, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	//sprite->AddAnimation(0, 0, 0, 24, 125, ANIM_LOOP_TYPES::LOOP_NONE);		// (9)  knocked backward
	//sprite->AddAnimation(0, 0, 0, 24, 125, ANIM_LOOP_TYPES::LOOP_NONE);		// (10) knocked forward
	sprite->AddAnimation(ANIMATION_DUCKING, 0, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_HANGING, 96, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_SLIDING, 0, 33, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_SHOOTING_HANGING, 64, 68, 2, 32, 80, ANIM_LOOP_TYPES::LOOP_NONE); // hang shoot
	sprite->AddAnimation(ANIMATION_SHOOTING_STANDING, 0, 68, 2, 32, 64, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_SHOOTING_JUMPING, 32, 33, 1, 0, 64, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_SHOOTING_FALLING, 64, 33, 1, 0, 64, ANIM_LOOP_TYPES::LOOP_NONE);
	sprite->AddAnimation(ANIMATION_SHOOTING_RUNNING, 128, 34, 4, 32, 150, ANIM_LOOP_TYPES::LOOP_NORMAL);
	state = new OnGroundState(this);
	InitPlayerTexture();
}

void Player::SwitchWeapon()
{
	bool switchDone = false;
	WEAPONS newWeap = weapon;
	while(!switchDone) // cycling through all the weapons until reaching the next one the player owns
	{
		newWeap = (WEAPONS)((newWeap + 1) % NUMWEAPONS);
		if(ownedWeapons[newWeap]) switchDone = true;
	}
	this->SwitchWeapon(newWeap);
}

void Player::SwitchWeapon(WEAPONS newWeap)
{
	if(ownedWeapons[newWeap]) weapon = newWeap;
	switch(weapon)
	{
		case WEAPON_FIREBALL:
		{
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 255, 0, 0, 255 });
			ChangePlayerColor(PLAYER_BODY_TAIL, { 255, 0, 0, 255 });
			break;
		}
		case WEAPON_LIGHTNING:
		{
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 0, 0, 255, 255 });
			ChangePlayerColor(PLAYER_BODY_TAIL, { 0, 0, 255, 255 });
			break;
		}
		default:
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 0, 0, 255, 255 });
	}
	ChangeInterfaceFrame(weapon, INTERFACE_ABILITY);
}

void Player::GiveWeapon(WEAPONS weap)
{
	ownedWeapons[weap] = true;

	switch(weap)
	{
		case WEAPON_ROCKETL:
			ammo[weap] += 10;
			break;
		case WEAPON_FLAME:
			ammo[weap] += 50;
			break;
		case WEAPON_GRENADE:
			ammo[weap] += 20;
			break;
	}
	SwitchWeapon(weap);
}

// required to initialize weapon array, putting in a function so we can maybe recycle it later?
void Player::ResetWeapons()
{
	for(int i = 0; i < NUMWEAPONS; i++)
	{
		ownedWeapons[i] = false;
		ammo[i] = 0;
	}

	fireDelay[WEAPON_ROCKETL] = (int)(0.7 * 1000);
	fireDelay[WEAPON_FLAME] = (int)(0.1 * 1000);
	//fireDelay[WEAPON_FLAME] = (int)(0.07 * 1000 );
	fireDelay[WEAPON_GRENADE] = (int)(1.0 * 1000);
	fireDelay[WEAPON_LIGHTNING] = (int)(0.5 * 3000);
	fireDelay[WEAPON_FIREBALL] = (int)(0.1 * 1000);
}

bool Player::CanMoveWhileFiring()
{
	if(shotLocked && weapon >= WEAPON_FLAME)
	{
		// TODO: velocity x 0 here
		return false;
	}
	return true;
}

void Player::ToggleChargedColor()
{
	chargedColored = !chargedColored;
	if(!chargedColored)
		ChangePlayerColor(PLAYER_BODY_SPECIAL, { 255, 255, 255, 255 });
	else
		DisableChargedColor();
}

void Player::DisableChargedColor()
{
	switch(weapon)
	{
		case WEAPON_FIREBALL:
		{
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 255, 0, 0, 255 });
			break;
		}
		case WEAPON_LIGHTNING:
		{
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 0, 0, 255, 255 });
			break;
		}
		default:
			ChangePlayerColor(PLAYER_BODY_SPECIAL, { 0, 0, 255, 255 });
	}
}

void Creature::ToggleDucking(bool enable)
{
	PrintLog(LOG_INFO, "ducking toggled");
	if(enable)
	{
		this->SetState(CREATURE_STATES::DUCKING);
		hitbox->SetRect({ 0, 0, 16, 16 });
		SetY(GetY());
		sprite->SetSpriteRect({ 0, 0, 16, 16 });
		sprite->SetSpriteOffset(0, -hitbox->GetRect().h);
	}
	else
	{
		hitbox->SetRect({ 0, 0, 15, 30 });
		SetY(GetY());
		sprite->SetSpriteRect({ 24, 0, 24, 32 });
		sprite->SetSpriteOffset(-hitbox->GetRect().w / 4, -hitbox->GetRect().h);
	}
}

Tripod::Tripod()
{
	hitbox = new Hitbox(0, 0, 1, 1); // dummy
	this->SetPos(0, 0);
	SetVelocity(0, 0);
	sprite = new Sprite(&player_texture, 0, 0, 0, 0); // dummy
}

Tripod::~Tripod()
{

}

Velocity DynamicEntity::GetVelocity()
{
	return velocity;
}

void DynamicEntity::SetVelocity(Velocity vel)
{
	velocity = vel;
}

void DynamicEntity::SetVelocity(double x, double y)
{
	velocity.x = x;
	velocity.y = y;
}

bool DynamicEntity::isMoving(bool onlyX)
{
	return (abs(velocity.x) > 0 || (!onlyX && abs(velocity.y) > 0));
}

void Creature::SetState(CREATURE_STATES state)
{
	CREATURE_STATES oldState = this->state->GetState();
	if(this->state != nullptr)
	{
		if(state == CREATURE_STATES::JUMPING && !(this->state->Is(CREATURE_STATES::ONGROUND) || this->state->Is(CREATURE_STATES::HANGING)))
			return;
		if(state != this->state->GetState())			
			delete this->state;
		else
			return;
	}
		
	switch(state)
	{
		case CREATURE_STATES::ONGROUND:
			this->state = new OnGroundState(this);
			break;
		case CREATURE_STATES::HANGING:
			this->state = new HangingState(this);
			break;
		case CREATURE_STATES::INAIR:
			this->state = new InAirState(this);
			break;
		case CREATURE_STATES::SLIDING:
			this->state = new SlidingState(this);
			break;
		case CREATURE_STATES::JUMPING:
			this->state = new JumpingState(this);
			break;
		case CREATURE_STATES::ONLADDER:
			this->state = new OnLadderState(this);
			break;
		case CREATURE_STATES::DUCKING:
			this->state = new DuckingState(this);
			break;
		default:
			PrintLog(LOG_IMPORTANT, "Creature state %d is not implemented!", state);
			this->state = new InAirState(this);
	}
	if(this->IsAI())
	{
		this->AI->OnStateChange(oldState, this->state->GetState());
	}
}

void Creature::SetState(CreatureState *newState)
{
	if(this->state != nullptr)
	{
		if(newState->GetState() == CREATURE_STATES::JUMPING && !(this->state->Is(CREATURE_STATES::ONGROUND) || this->state->Is(CREATURE_STATES::HANGING)))
		{
			delete newState;
			return;
		}
		if(newState->GetState() != this->state->GetState())
			delete this->state;
		else
		{
			delete newState;
			return;
		}
	}
	state = newState;
}

void Creature::HandleInput(int input, int type)
{
	if(player->status == STATUS_STUN)
		return;

	CreatureState *newState = this->state->HandleInput(input, type);
	if(newState != nullptr)
	{
		player->SetState(newState);
	}
}

void Creature::HandleStateIdle()
{
	CreatureState *newState = this->state->HandleIdle();
	if(newState != nullptr)
	{
		player->SetState(newState);
	}
}

void Entity::SwitchDirection()
{
	if(direction == DIRECTION_LEFT)
		direction = DIRECTION_RIGHT;
	else
		direction = DIRECTION_LEFT;
}

void Entity::SetDirection(DIRECTIONS direction)
{
	this->direction = direction;
}

void Creature::Walk()
{
	Walk(direction);
}

void Creature::Walk(DIRECTIONS direction)
{
	Velocity vel;
	vel = this->GetVelocity();
	if(!this->state->Is(CREATURE_STATES::ONLADDER))
		this->direction = direction;

	if(this->state->Is(CREATURE_STATES::ONGROUND) && !this->state->Is(CREATURE_STATES::ONLADDER)) {
		this->accel.x = 1 * (direction ? 1 : -1);
		if(IsOnIce(*this))
			this->accel.x = 0.35 * (direction ? 1 : -1);
	}
	else if(!this->state->Is(CREATURE_STATES::ONGROUND))
		this->accel.x = 0.7 * (direction ? 1 : -1);

	this->SetVelocity(vel.x, vel.y);
}

Creature::Creature(std::string type)
{
	if(creatureData.find(type) == creatureData.end())
	{
		PrintLog(LOG_DEBUG, "Invalid creature type %s!", type);
		delete this;
		return;
	}
	health = creatureData[type].health;
	ignoreGravity = creatureData[type].ignoreGravity;
	ignoreWorld = creatureData[type].ignoreWorld;
	gravityMultiplier = creatureData[type].gravityMultiplier;
	move_vel = creatureData[type].move_vel;
	term_vel = creatureData[type].term_vel;
	weapon = creatureData[type].weapon;
	blinkDamaged = creatureData[type].blinkDamaged;
	//weapon = WEAPON_ROCKETL;

	attached = false;
	status = 0;
	AI = nullptr;
	nearladder = false;
	nearhook = false;
	lefthook = false;
	charge_time = 0;
	shottime = 0;
	accel.x = 0;
	accel.y = 0;
	jumptime = 0;
	statusTimer = 0;
	shotLocked = false;
	charging = false;
	onMachinery = false;

	creatures.push_back(this);
	entityID = AssignEntityID(LIST_CREATURES);

	direction = DIRECTION_RIGHT;

	SetVelocity(0, 0);

	std::string graphicsName = creatureData[type].graphicsName;
	hitbox = new Hitbox(creatureGraphicsData[graphicsName].hitbox);
	sprite = new Sprite(creatureGraphicsData[graphicsName].sprite);
	state = new InAirState(this);
}

Creature::Creature()
{
	AI = nullptr;
	nearladder = false;
	nearhook = false;
	lefthook = false;
	charge_time = 0;
	shottime = 0;
	accel.x = 0;
	accel.y = 0;
	blinkDamaged = true;
	shotLocked = false;
	charging = false;
	onMachinery = false;

	state = new InAirState(this);
	// creatures are leaking a few bytes when created, something to do with DamageSource it seems like?
}

Creature::~Creature()
{
	if(AI != nullptr)
		delete AI;
	auto i = std::find(creatures.begin(), creatures.end(), this);
	if(i != creatures.end()) *i = nullptr;
	// attempts to fix Creature memory leak above (didn't seem to work)
	//hitFrom.clear();
	//hitFrom.~vector();
}

bool Creature::IsAI()
{
	return !(this->AI == nullptr);
}

void Entity::GetPos(double &x, double &y)
{
	x = this->x;
	y = this->y;
}

void Entity::GetPos(int &x, int &y)
{
	x = (int)round(this->x);
	y = (int)round(this->y);
}

void Entity::SetPos(int x, int y)
{
	hitbox->SetPos(x, y - hitbox->GetRect().h);
	this->x = x;
	this->y = y;
}

void Entity::SetPos(double x, double y)
{
	hitbox->SetPos(x, y - hitbox->GetRect().h);
	this->x = x;
	this->y = y;
}

int Entity::AssignEntityID(int vectorID)
{
	bool taken = false;

	switch(vectorID)
	{
		case LIST_BULLETS:
			for(int i = 0; i < (int)bullets.size(); i++)
			{
				for(Bullet* j : bullets)
				{
					if(j->entityID == i)
						taken = true;
				}
				if(!taken)
					return i;
				else
					taken = false;
			}
			return static_cast<int>(bullets.size());

		case LIST_CREATURES:
			for(int i = 0; i < (int)creatures.size(); i++)
			{
				for(Creature* j : creatures)
				{
					if(j->entityID == i)
						taken = true;
				}
				if(!taken)
					return i;
				else
					taken = false;
			}
			return static_cast<int>(creatures.size());

		case LIST_MACHINERY:
			for(int i = 0; i < (int)machinery.size(); i++)
			{
				for(Machinery* j : machinery)
				{
					if(j->entityID == i)
						taken = true;
				}
				if(!taken)
					return i;
				else
					taken = false;
			}
			return static_cast<int>(machinery.size());

		case LIST_PICKUPS:
			for(int i = 0; i < (int)pickups.size(); i++)
			{
				for(Pickup* j : pickups)
				{
					if(j->entityID == i)
						taken = true;
				}
				if(!taken)
					return i;
				else
					taken = false;
			}
			return static_cast<int>(pickups.size());

		case LIST_EFFECTS:
			for(int i = 0; i < (int)effects.size(); i++)
			{
				for(Effect* j : effects)
				{
					if(j->entityID == i)
						taken = true;
				}
				if(!taken)
					return i;
				else
					taken = false;
			}
			return static_cast<int>(effects.size());

		default:
		{
			//todo: replace with assert?
			PrintLog(LOG_IMPORTANT, "critical entity ID failure ouch");
			return -1; // should never happen
		}
	}
}

double Entity::GetDistanceToEntity(Entity *e)
{
	return sqrt(pow(this->GetX() - e->GetX(), 2) + pow(this->GetY() - e->GetY(), 2));
}

double Entity::GetXDistanceToEntity(Entity *e)
{
	return abs(this->GetX() - e->GetX());
}

double Entity::GetYDistanceToEntity(Entity *e)
{
	return abs(this->GetY() - e->GetY());
}

Hitbox::Hitbox(double x, double y, double h, double w)
{
	this->x = x;
	this->y = y;
	this->h = h;
	this->w = w;
}

bool Hitbox::HasCollision(Hitbox *hitbox)
{
	return !(this->x > hitbox->x + hitbox->w || this->x + this->w < hitbox->x ||
		this->y > hitbox->y + hitbox->h || this->y + this->h < hitbox->y);
}

SDL_Rect Hitbox::GetRect()
{
	SDL_Rect r;
	r.h = (int)ceil(h - 0.50001);
	r.w = (int)ceil(w - 0.50001);
	r.x = (int)ceil(x - 0.50001);
	r.y = (int)ceil(y - 0.50001);
	return r;
}

void Hitbox::SetSize(int w, int h)
{
	this->w = w;
	this->h = h;
}

PrecisionRect Hitbox::GetPRect()
{
	PrecisionRect r;
	r.h = h;
	r.w = w;
	r.x = x;
	r.y = y;
	return r;
}

void Hitbox::SetPos(double x, double y)
{
	this->x = x;
	this->y = y;
}

void Hitbox::SetRect(SDL_Rect rect)
{
	x = rect.x;
	y = rect.y;
	w = rect.w;
	h = rect.h;
}

void Pickup::OnPickup()
{
	if(this->type != PICKUP_NOTHING)
	{
		this->status = STATUS_DYING;
		this->statusTimer = this->deathLength;

		switch(this->type)
		{
			case PICKUP_AMMO:
				// TODO: Find who picked up
				player->ammo[player->weapon] += 10;
				PlaySound("pickup_ammo");
				//PrintNumToInterface(player.ammo, INTERFACE_AMMO, 3);
				break;
			case PICKUP_HEALTH:
				player->health += 50;
				if(player->health > 100) player->health = 100;
				break;
			case PICKUP_LIGHTNING:
				player->GiveWeapon(WEAPON_LIGHTNING);
				break;
			case PICKUP_FIREBALL:
				player->GiveWeapon(WEAPON_FIREBALL);
				break;
			case PICKUP_GRENADE:
				player->GiveWeapon(WEAPON_GRENADE);
				break;
			default:
				break;
		}
	}
}

// default dummy constructor yay
Pickup::Pickup()
{

}

Pickup::Pickup(PICKUP_TYPES spawnType)
{
	pickups.push_back(this);
	entityID = AssignEntityID(LIST_PICKUPS);

	type = spawnType;

	SDL_Point offset;
	switch(spawnType)
	{
		case PICKUP_AMMO:
			offset = { 16, 0 };
			break;
		case PICKUP_HEALTH:
			offset = { 0 + 8, 0 };
			break;
		case PICKUP_LIGHTNING:
			offset = { 64 + 8, 0 };
			break;
		case PICKUP_FIREBALL:
			offset = { 32 + 8, 0 };
			break;
		case PICKUP_GRENADE:
			offset = { 64, 0 };
			break;
	}

	sprite = new Sprite(textureManager.GetTexture("assets/sprites/pickups.png"), offset.x, offset.y, 28, 32);
	sprite->SetSpriteOffset(0, -28);
	hitbox = new Hitbox(0, 0, 28, 32);
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 24, 125, ANIM_LOOP_TYPES::LOOP_PINGPONG);
	deathLength = 0;
	direction = DIRECTION_RIGHT;
}

void DynamicEntity::Remove()
{
	delete this;
}

void Pickup::Remove()
{
	delete this;
}

void Effect::Remove()
{
	delete this;
}

void Bullet::Remove()
{
	if(origin == WEAPON_FIREBALL && owner == player)
		player->ammo[WEAPON_FIREBALL]++;
	delete this;
}

Player::~Player()
{

}

DynamicEntity::~DynamicEntity()
{
	if(camera && camera->IsAttachedTo(this))
		camera->Detach();
	delete hitbox;
	delete sprite;
	hitbox = NULL;
	sprite = NULL;
}

Pickup::~Pickup()
{
	pickups.erase(std::remove(pickups.begin(), pickups.end(), this), pickups.end());
	delete hitbox;
	delete sprite;
	hitbox = NULL;
	sprite = NULL;
}

Effect::~Effect()
{
	effects.erase(std::remove(effects.begin(), effects.end(), this), effects.end());
	delete hitbox;
	delete sprite;
	hitbox = NULL;
	sprite = NULL;
}

Effect::Effect()
{
	effects.push_back(this);
	entityID = AssignEntityID(LIST_EFFECTS);

	hitbox = new Hitbox(0, 0, 8, 8);
	sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 8, 8);
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 8, 125, ANIM_LOOP_TYPES::LOOP_NONE);
}

Effect::Effect(EFFECT_TYPES type)
{
	effects.push_back(this);
	entityID = AssignEntityID(LIST_EFFECTS);

	switch(type)
	{
		case EFFECT_MGUN_HIT:
			hitbox = new Hitbox(0, 0, 8, 8);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 8, 8);
			sprite->SetSpriteOffset(0, -8);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 8, 94, ANIM_LOOP_TYPES::LOOP_NONE);
			status = STATUS_DYING;
			statusTimer = (int)(0.4 * 1000);
			break;
		case EFFECT_ROCKETL_HIT:
			hitbox = new Hitbox(0, 0, 28, 28);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/effect_rocketl.png"), 0, 0, 28, 28);
			sprite->SetSpriteOffset(0, -28);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 3, 28, 100, ANIM_LOOP_TYPES::LOOP_NONE);
			status = STATUS_DYING;
			statusTimer = (int)(0.3 * 1000);
			break;
		case EFFECT_ZAP:
			hitbox = new Hitbox(0, 0, 6, 1);
			sprite = new Sprite(NULL, 0, 0, 6, 1);
			sprite->SetSpriteOffset(0, -26);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 999, 100, ANIM_LOOP_TYPES::LOOP_NONE);
			status = STATUS_DYING;
			statusTimer = (int)(1 * 1000);
			break;
		default:
			hitbox = new Hitbox(0, 0, 8, 8);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 8, 8);
			sprite->SetSpriteOffset(0, -8);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 8, 94, ANIM_LOOP_TYPES::LOOP_NONE);
			break;
	}
	direction = DIRECTION_RIGHT;
}

Bullet::Bullet()
{
	bullets.push_back(this);
	entityID = AssignEntityID(LIST_BULLETS);

	hitbox = new Hitbox(0, 0, 8, 8);
	sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 8, 8);
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 8, 125, ANIM_LOOP_TYPES::LOOP_NONE);

	origin = WEAPON_FLAME;
	direction = DIRECTION_RIGHT;
}

// Bullets are leaking memory and I don't know why
Bullet::Bullet(WEAPONS firedFrom, Creature &shooter)
{
	bullets.push_back(this);
	entityID = AssignEntityID(LIST_BULLETS);
	owner = &shooter;
	direction = shooter.direction;

	std::pair<double, double> angles = GetAngleSinCos(shooter);

	switch(firedFrom)
	{
		case WEAPON_ROCKETL:
			hitbox = new Hitbox(0, 0, 4, 7);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/rocketlbullet.png"), 0, 2, 4, 7);
			sprite->SetSpriteOffset(0, -4);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
			SetVelocity(400 * (direction ? 1 : -1), 0);
			accel.y = 0;
			accel.x = 0;
			lifetime = (int)(1 * 1000);
			statusTimer = lifetime;
			piercing = false;
			break;
		case WEAPON_FLAME:
			hitbox = new Hitbox(0, 0, 12, 8);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/flamebullet.png"), 0, 0, 14, 8);
			sprite->SetSpriteOffset(0, -12);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 8, 105, ANIM_LOOP_TYPES::LOOP_NONE);
			SetVelocity(this->owner->GetVelocity().x + 80 * (direction ? 1 : -1), 0);
			accel.y = -2.5;
			accel.x = 0;
			lifetime = (int)(0.3 * 1000);
			statusTimer = lifetime;
			piercing = false;
			break;
		case WEAPON_GRENADE:
			hitbox = new Hitbox(0, 0, 7, 7);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/rocketlbullet.png"), 0, 0, 7, 7);
			sprite->SetSpriteOffset(0, -16);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 7, 105, ANIM_LOOP_TYPES::LOOP_NORMAL);
			if(shooter.state->Is(CREATURE_STATES::ONLADDER))
				SetVelocity(0, 0);
			else
				SetVelocity(150 * (direction ? 1 : -1), -200);
			accel.y = 5;
			accel.x = 0;
			lifetime = (int)(1.8 * 1000);
			statusTimer = lifetime;
			piercing = false;
			break;
		case WEAPON_FIREBALL:
			hitbox = new Hitbox(0, 0, 7, 9);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 16, 16);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 16, 90, ANIM_LOOP_TYPES::LOOP_NONE);
			sprite->SetSpriteOffset(-3, -12);
			SetVelocity(240 * (direction ? 1 : -1), 0);
			accel.y = 0;
			accel.x = 0;
			lifetime = (int)(1 * 2000);
			statusTimer = lifetime;
			piercing = false;
			break;
		case WEAPON_BOMBDROP:
			hitbox = new Hitbox(0, 0, 16, 16);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/bombdrop.png"), 0, 0, 16, 16);
			sprite->SetSpriteOffset(0, -16);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 0, 125, ANIM_LOOP_TYPES::LOOP_NONE);
			SetVelocity(0, 100);
			accel.y = 0.1;
			accel.x = 0;
			lifetime = (int)(15 * 1000);
			statusTimer = lifetime;
			piercing = false;
			break;
		case WEAPON_GROUNDSHOCKWAVE:
			hitbox = new Hitbox(0, 0, 7, 9);
			sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 16, 16);
			sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 4, 16, 90, ANIM_LOOP_TYPES::LOOP_NONE);
			sprite->SetSpriteOffset(-3, -12);
			SetVelocity(240 * (direction ? 1 : -1), 0);
			accel.y = 0;
			accel.x = 0;
			lifetime = (int)(0.5 * 1000);
			statusTimer = lifetime;
			piercing = false;
			break;
	}
	origin = firedFrom;
}

Bullet::~Bullet()
{
	auto i = std::find(bullets.begin(), bullets.end(), this);
	if(i != bullets.end()) *i = nullptr;
}

void Creature::ProcessBulletHit(Bullet *b)
{
	if(b->piercing)
	{
		for(auto &j : hitFrom)
		{
			if(j.id == b->entityID) return;
		}

		// creature hasn't been hit by this bullet yet, mark creature as hit
		DamageSource d;
		d.id = b->entityID;
		d.immunity = b->statusTimer;
		hitFrom.push_back(d);
	}

	int damage = 0;
	switch(b->origin)
	{
		case WEAPONS::WEAPON_ROCKETL:
		{
			damage = 50;
			break;
		}
		case WEAPONS::WEAPON_FLAME:
		{
			damage = 30;
			break;
		}
		case WEAPONS::WEAPON_GRENADE:
		{
			damage = 50;
			break;
		}
		case WEAPONS::WEAPON_FIREBALL:
		{
			damage = 50;
			break;
		}
		case WEAPONS::WEAPON_BOMBDROP:
		{
			damage = 25;
			break;
		}
		case WEAPONS::WEAPON_GROUNDSHOCKWAVE:
		{
			damage = 25;
			break;
		}
	}
	this->TakeDamage(damage);
	if(this == player)
		ApplyKnockback(*this, *(Creature*)b->owner);
}

void Creature::SetStun(int milliseconds)
{
	status = STATUS_STUN;
	statusTimer = milliseconds;
}

void Creature::SetInvulnerability(int milliseconds)
{
	status = STATUS_INVULN;
	statusTimer = milliseconds;
}

void Creature::TakeDamage(int damage)
{
	if(status == STATUS_DYING || status == STATUS_INVULN || status == STATUS_STUN)
		return;
	health -= damage;
	SetInvulnerability(500);
	if(health <= 0)
	{
		PlaySound("death");
		if(this != player) // TODO: remove this walkaround
			this->Die();
	}
}

void Machinery::Activate()
{
	// only contains door logic for now
	// TODO: call door open and close functions somehow (I don't think they're being used yet)
	if(enabled)
	{
		SetVelocity(0, 200);
		PlaySound("door_close");
		enabled = false;
	}
	else
	{
		SetVelocity(0, -200);
		PlaySound("door_open");
		enabled = true;
	}
}

Door::Door(int x, int y, bool spawnButtons)
{
	machinery.push_back(this);
	entityID = AssignEntityID(LIST_MACHINERY);

	default_pos.x = x;
	default_pos.y = y;
	default_pos.h = 16 * 3;
	default_pos.w = 16;
	hitbox = new Hitbox(0, 0, default_pos.h, default_pos.w);
	sprite = new Sprite(textureManager.GetTexture("assets/textures/tiles1.png"), 10 * 16, 9 * 16, default_pos.h, default_pos.w);
	if(spawnButtons)
	{
		pairID = doorPairs;
		leftButton = new Button(x - 32, y + 16, pairID);
		rightButton = new Button(x + 32, y + 16, pairID);
		doorPairs++;
	}
	SetPos(x, y);
	SetVelocity(0, 0);
	isSolid = true;
	enabled = false;
	destructable = true;
	automatic = false;
	direction = DIRECTION_RIGHT;
}

Machinery::~Machinery()
{
	machinery.erase(std::remove(machinery.begin(), machinery.end(), this), machinery.end());
}

// dummy destructor (don't delete or game will crash when deleting doors)
Door::~Door()
{

}

void Door::Remove()
{
	delete this;
}

void Door::Open()
{
	SetVelocity(0, -200);
	PlaySound("door_open");
}

void Door::Close()
{
	SetVelocity(0, 200);
	PlaySound("door_close");
}

Button::Button(int x, int y, int doorID)
{
	machinery.push_back(this);
	entityID = AssignEntityID(LIST_MACHINERY);

	default_pos.x = x;
	default_pos.y = y;
	default_pos.h = 32;
	default_pos.w = 16;
	hitbox = new Hitbox(0, 0, default_pos.h, default_pos.w);
	sprite = new Sprite(textureManager.GetTexture("assets/textures/tiles1.png"), 10 * 16, 7 * 16, default_pos.h, default_pos.w);
	pairID = doorID;
	SetPos(x, y);
	SetVelocity(0, 0);
	isSolid = false;
	automatic = false;
	destructable = false;
	direction = DIRECTION_RIGHT;
}

Button::~Button()
{
	machinery.erase(std::remove(machinery.begin(), machinery.end(), this), machinery.end());
	delete hitbox;
	delete sprite;
	hitbox = NULL;
	sprite = NULL;
}

// not currently being used
// todo: this should really be used
void Button::Activate()
{
	/*if (door->enabled)
		door->Close();
	else
		door->Open();*/
}

void Button::Remove()
{
	delete this;
}

Platform::Platform(int x, int y, int x2, int y2)
{
	machinery.push_back(this);
	entityID = AssignEntityID(LIST_MACHINERY);

	if(x > x2)
	{
		std::swap(x, x2);
		std::swap(y, y2);
	}
	else if(y > y2)
	{
		std::swap(x, x2);
		std::swap(y, y2);
	}
	default_pos.x = x;
	default_pos.y = y;
	default_pos.h = 16;
	default_pos.w = 16 * 2;
	another_pos.x = x2;
	another_pos.y = y2;
	another_pos.h = 16;
	another_pos.w = 16 * 2;
	hitbox = new Hitbox(0, 0, default_pos.h, default_pos.w);
	sprite = new Sprite(textureManager.GetTexture("assets/textures/tiles1.png"), 6 * 16, 11 * 16, default_pos.h, default_pos.w);
	sprite->SetSpriteOffset(0, -default_pos.h);
	SetPos(x, y + 16);
	SetVelocity(0, 0);
	speed = 50;
	minspeed = 10;
	deaccel.x = speed * 1.85;
	deaccel.y = speed * 1.15;
	isSolid = false;
	automatic = true;
	destructable = false;
	direction = DIRECTION_RIGHT;
}

void Platform::Remove()
{
	delete this;
}

Platform::~Platform()
{
	machinery.erase(std::remove(machinery.begin(), machinery.end(), this), machinery.end());
	delete hitbox;
	delete sprite;
	hitbox = NULL;
	sprite = NULL;
}

// otherwise known as "destroy all the machinery in the level"
void TestMemory()
{
	for(int i = 0; i < 100; i++)
	{
		Door *d = new Door(i, i, false);
		delete d;
	}

	PrintLog(LOG_INFO, "Hmm");

	for(int i = 0; i < 100; i++)
	{
		Door *d = new Door(i, i, false);
		delete d;
	}

	for(int t = machinery.size() - 1; t >= 0; t--)
	{
		delete machinery.at(t);
	}

	PrintLog(LOG_INFO, "HMm");
	for(int i = 0; i < 100; i++)
	{
		Door *d = new Door(i, i, false);
		delete d;
	}

	PrintLog(LOG_INFO, "HmM");

	for(int t = machinery.size() - 1; t >= 0; t--)
	{
		delete (Door*)machinery.at(t);
	}
}

void DeleteAllEntities()
{
	for(int t = creatures.size() - 1; t >= 0; t--)
		creatures.at(t)->Remove();
	creatures.clear();

	for(int t = bullets.size() - 1; t >= 0; t--)
		bullets.at(t)->Remove();
	bullets.clear();

	for(int t = machinery.size() - 1; t >= 0; t--)
		machinery.at(t)->Remove();
	machinery.clear();

	for(int t = pickups.size() - 1; t >= 0; t--)
		delete pickups.at(t);
	pickups.clear();

	for(int t = effects.size() - 1; t >= 0; t--)
		delete effects.at(t);
	effects.clear();

	for(int t = lightnings.size() - 1; t >= 0; t--)
		delete lightnings.at(t);
	lightnings.clear();
}

void EntityCleanup()
{
	creatures.~vector();
	bullets.~vector();
	machinery.~vector();
	pickups.~vector();
	effects.~vector();
	lightnings.~vector();
	delete ai_rg;
	ai_rg = NULL;
}

void Creature::MoveUp()
{
	this->accel.y = -0.7;
}

void Creature::MoveDown()
{
	this->accel.y = 0.7;
}

void Creature::Shoot()
{
	ProcessShot(weapon, *this);
}

void Creature::Die()
{
	if(AI != nullptr)
	{
		delete AI;
		AI = nullptr;
	}
	SetState(CREATURE_STATES::INAIR);
	status = STATUS_DYING;
	ignoreWorld = true;
	ignoreGravity = false;
	term_vel = 360;
	accel.x = 0;
	accel.y = 0;
	gravityMultiplier = 1;
	if(player->GetX() > GetX())
		this->SetVelocity(-60, -150);
	else
		this->SetVelocity(60, -150);
}

Lightning::Lightning()
{
	lightnings.push_back(this);

	hitbox = new Hitbox(0, 0, 8, 8);
	sprite = new Sprite(textureManager.GetTexture("assets/sprites/fireball.png"), 0, 0, 8, 8);
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 8, 125, ANIM_LOOP_TYPES::LOOP_NONE);
}

std::vector<SDL_Point> CalcLightningPoints(SDL_Point from, DIRECTIONS direction)
{
	std::vector<SDL_Point> points;
	std::vector<LightningBranch> branches;

	int maxBranches = 3;
	int maxSpread = 9;
	bool isAlive = true;

	branches.push_back(LightningBranch{ 0, 0, 0, false, 20, 150 });
	int i = 0;

	while(isAlive)
	{
		isAlive = false;
		for(auto &l : branches)
		{
			if(l.lifetime < 1) continue;

			if(ai_rg->Generate(1, 10) == 1 && (int)branches.size() < maxBranches && l.timeUntilBranchable < 0)
				l.forkPending = true;

			if(l.forkEscape > 0)
			{
				l.going = -1;
				l.forkEscape--;
			}
			else if(l.forkEscape < 0)
			{
				l.going = 1;
				l.forkEscape++;
			}
			else
			{
				int rand = ai_rg->Generate(1, 8);
				if(rand < 4) l.going = -1;
				if(rand >= 4 && rand <= 5) l.going = 0;
				if(rand > 5) l.going = 1;
			}

			l.offset += l.going;
			if(l.offset > maxSpread)
				l.offset = maxSpread;
			if(l.offset < maxSpread * -1)
				l.offset = maxSpread * -1;

			l.timeUntilBranchable--;
			int taperOff = l.lifetime < 10 ? 512 : l.lifetime < 30 ? 256 : 0;
			points.push_back(SDL_Point{ i, 10 + l.offset + taperOff });

			SDL_Point curPos;
			curPos = { from.x + i, from.y + l.offset };

			switch(GetTileTypeAtPos(curPos))
			{
				case PHYSICS_BLOCK: case PHYSICS_ICE: case PHYSICS_ICEBLOCK: case PHYSICS_OB:
				{
					l.lifetime = 0;
					break;
				}
				case PHYSICS_RAIN:
				{
					l.lifetime--;
					break;
				}
				default:
				{
					l.lifetime -= 2;
				}
			}

			if(l.lifetime > 0) isAlive = true;
		}

		int branchFrom = -1;
		int way = ai_rg->Generate(1, 2) == 1 ? 2 : -2;
		for(auto &l : branches)
		{
			if(l.forkPending)
			{
				if(abs(l.offset) > maxSpread - 2)
				{
					l.forkPending = false;
					continue;
				}
				branchFrom = &l - &branches[0];
				l.forkEscape = way * -1;
				l.timeUntilBranchable = 30;
				l.forkPending = false;
			}
		}
		if(branchFrom > -1)
			branches.push_back(LightningBranch{ branches.at(branchFrom).offset,0,way,false,30,std::min(branches.at(0).lifetime + 10,30) });

		if(!direction)
			i--;
		else
			i++;
	}

	if(!direction)
	{
		for(auto &p : points)
			p.x = 0 - p.x;
	}
	return points;
}

Lightning::Lightning(DynamicEntity &shooter)
{
	lightnings.push_back(this);
	owner = &shooter;
	direction = shooter.direction;

	std::pair<double, double> angles = GetAngleSinCos(shooter);

	int a = SDL_GetTicks();

	// currently magic size numbers
	hitbox = new Hitbox(0, 0, 22, 150);

	SDL_Point posFrom = { (int)shooter.GetX() + 15, (int)shooter.GetY() - 20 };
	if(!direction)
		posFrom = { (int)shooter.GetX(), (int)shooter.GetY() - 20 };

	std::vector<SDL_Point> points = CalcLightningPoints(posFrom, shooter.direction);
	tex = GenerateLightningTexture(points);

	int width, height;
	SDL_QueryTexture(tex, NULL, NULL, &width, &height);
	hitbox->SetSize(width, height);
	sprite = new Sprite(&tex, 0, 0, height, width);
	sprite->SetSpriteOffset(0, -height + 3);

	// placing hitbox where it should be
	double x, y;
	y = shooter.GetY() - 7;
	if(!direction)
		x = shooter.GetX() - width - 3;
	else
		x = shooter.GetX() + 15;
	this->SetPos(x, y);

	// dummy I guess
	sprite->AddAnimation(ANIMATION_STANDING, 0, 0, 1, 0, 90, ANIM_LOOP_TYPES::LOOP_NONE);

	SetVelocity(0, 0);
	status = STATUS_INVULN;
	lifetime = (int)(1 * 500);
	statusTimer = lifetime;
	piercing = true;
	PrintLog(LOG_SUPERDEBUG, "%d TIME PASSED", SDL_GetTicks() - a);
}

void Lightning::Remove()
{
	delete this;
}

Lightning::~Lightning()
{
	if(this->tex != nullptr)
		SDL_DestroyTexture(this->tex);
	lightnings.erase(std::remove(lightnings.begin(), lightnings.end(), this), lightnings.end());
}

Lava_Floor::Lava_Floor(int x, int y)
{
	machinery.push_back(this);
	default_pos.x = x;
	default_pos.y = y;
	default_pos.h = 16;
	default_pos.w = 256;
	hitbox = new Hitbox(0, 0, default_pos.h, default_pos.w);
	sprite = new Sprite(textureManager.GetTexture("assets/sprites/lava_floor.png"), 0, 32, default_pos.h, default_pos.w);
	SetPos(x, y);
	SetVelocity(0, 0);
	isSolid = false;
	automatic = false;
	destructable = false;
	enabled = false;
	direction = DIRECTION_RIGHT;
	attachToScreen = true;
	attachScreenX = 0;
	attachScreenY = 14 * 16;
}

void Lava_Floor::Remove()
{
	delete this;
}

void Lava_Floor::Activate()
{

}

Lava_Floor::~Lava_Floor()
{
	machinery.erase(std::remove(machinery.begin(), machinery.end(), this), machinery.end());
}