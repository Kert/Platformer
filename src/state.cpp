#include "state.h"
#include "config.h"
#include "input.h"
#include "physics.h"
#include "utils.h"

extern std::vector<Machinery*> machinery;

const int SHOOTING_ANIM_DURATION = 30;
EntityState* EntityState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0: // press
		{
			switch(input)
			{
				case BIND_FIRE:
					if(p.shottime == 0)
					{
						p.shottime = p.fireDelay[p.weapon];
						if(p.ammo[p.weapon] && p.weapon == WEAPON_GRENADE)
						{
							p.ammo[p.weapon]--;
							ProcessShot(p.weapon, *pl);
						}
						if(p.weapon == WEAPON_LIGHTNING)
						{
							ProcessShot(p.weapon, *pl);
						}
						if(p.ammo[p.weapon] && p.weapon == WEAPON_FIREBALL)
						{
							p.ammo[p.weapon]--;
							p.setState(STATE_CHARGING);
							ProcessShot(p.weapon, *pl);
							p.charge_time = 700;
						}
						p.sprite->shootingAnimTimer = SHOOTING_ANIM_DURATION;
					}
					break;
				case BIND_SWITCH:
					//if(!p.hasState(STATE_SHOOTING))
					//	p.SwitchWeapon();
					break;
			}
			break;
		}
		case 1: // hold
		{
			switch(input)
			{
				case BIND_FIRE:
					/*if (pl->shottime == 0)
					{
						if (pl->weapon == WEAPON_MGUN)
							pl->setState(STATE_SHOOTING);
					}*/
					//if (pl->hasState(STATE_SHOOTING))
					//{
					//	if (pl->ammo[pl->weapon] && !pl->hasState(STATE_ONLADDER) && (pl->weapon != WEAPON_GRENADE))
					//	{
					//		pl->ammo[pl->weapon]--;
					//		ProcessShot(pl->weapon, *pl);
					//		pl->removeState(STATE_SHOOTING);
					//		pl->shottime = pl->fireDelay[pl->weapon];
					//		if (pl->weapon >= 2)
					//			pl->setState(STATE_SHOTLOCKED); // don't let player move while firing fire
					//	}
					//}
					break;
			}
			break;
		}
		case 2: // unpress
		{
			switch(input)
			{
				case BIND_FIRE:
					if(pl->weapon == WEAPON_FIREBALL)
					{
						if(pl->hasState(STATE_CHARGING) && pl->charge_time == 0)
						{
							if(!IsInRain(p))
							{
								ProcessShot(WEAPON_ROCKETL, *pl); // charged
								pl->DisableChargedColor();
								p.sprite->shootingAnimTimer = SHOOTING_ANIM_DURATION;
							}
						}
					}
					pl->charge_time = 0;
					pl->removeState(STATE_CHARGING);
					break;
			}
			break;
		}
	}
	return nullptr;
}

EntityState* EntityState::HandleIdle(Player &p)
{
	return nullptr;
}

void EntityState::Initialize(Player &p)
{
	pl = &p;
	Enter(p);
}

EntityState::~EntityState()
{

}

void NormalState::Enter(Player &p)
{
	PrintLog(LOG_DEBUG, "Switched to ONGROUND");
}

EntityState* NormalState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0:
		{
			switch(input)
			{
				case BIND_DOWN:
					//if (!p.nearladder && p.hasState(STATE_ONGROUND))
					//	return new DuckingState();
					if(IsOnIce(p))
						if((p.GetVelocity().x > 80 && p.direction) || (p.GetVelocity().x < -80 && !p.direction))// && (IsBindPressed(BIND_LEFT) || IsBindPressed(BIND_RIGHT)))
							return new SlidingState();
					break;
				case BIND_UP:
				{
					if(p.interactTarget > -1)
					{
						for(auto &dy : machinery)
						{
							if(p.interactTarget == dy->pairID && dy->isSolid == true)
								dy->Activate();
						}
					}
					if(p.nearladder)
						return new OnLadderState();
					break;
				}
				case BIND_JUMP:
					if(p.hasState(STATE_ONGROUND))
						return new JumpingState();
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 1: // hold
		{
			switch(input)
			{
				/*case BIND_DOWN:
					if (!p.nearladder && p.hasState(STATE_ONGROUND))
						return new DuckingState();
					break;*/
				case BIND_RIGHT:
					if(IsBindPressed(BIND_DOWN) && IsOnIce(p))
						if((p.GetVelocity().x > 100 && p.direction) || (p.GetVelocity().x < -100 && !p.direction))// && (IsBindPressed(BIND_LEFT) || IsBindPressed(BIND_RIGHT)))
							return new SlidingState();
					if(pl->status == STATUS_DYING)
						break;
					pl->SetDirection(DIRECTION_RIGHT);
					pl->Walk();
					break;
				case BIND_LEFT:
					if(IsBindPressed(BIND_DOWN) && IsOnIce(p))
						if((p.GetVelocity().x > 100 && p.direction) || (p.GetVelocity().x < -100 && !p.direction))// && (IsBindPressed(BIND_LEFT) || IsBindPressed(BIND_RIGHT)))
							return new SlidingState();
					if(pl->status == STATUS_DYING)
						break;
					pl->SetDirection(DIRECTION_LEFT);
					pl->Walk();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 2: //unpress
		{
			switch(input)
			{
				case BIND_RIGHT: case BIND_LEFT:
					p.accel.x = 0;
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
	}
	return nullptr;
}

void OnLadderState::Enter(Player &p)
{
	PrintLog(LOG_DEBUG, "Switched to LADDER");
	p.setState(STATE_ONLADDER);
	p.direction = DIRECTION_RIGHT;
}

OnLadderState::~OnLadderState()
{
	pl->removeState(STATE_ONLADDER);
}

EntityState* OnLadderState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0:
		{
			switch(input)
			{
				case BIND_LEFT:
					break;
				case BIND_RIGHT:
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 1: // hold
		{
			switch(input)
			{
				case BIND_UP:
					p.SetVelocity(0, -p.climb_vel);
					if(!p.nearladder)
						return new NormalState();
					break;
				case BIND_DOWN:
					p.SetVelocity(0, p.climb_vel);
					if(!p.nearladder || p.hasState(STATE_ONGROUND))
						return new NormalState();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 2:
		{
			switch(input)
			{
				case BIND_UP: case BIND_DOWN:
					p.SetVelocity(0, 0);
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
	}
	return nullptr;
}

void DuckingState::Enter(Player &p)
{
	PrintLog(LOG_DEBUG, "Switched to DUCKING");
	pl->ToggleDucking(true);
	pl->accel.x = 0;
}

DuckingState::~DuckingState()
{
	pl->ToggleDucking(false);
}

EntityState* DuckingState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0: // press
		{
			switch(input)
			{
				case BIND_RIGHT:	case BIND_LEFT: case BIND_UP:
					p.ToggleDucking(false);
					return new NormalState();
					break;
				case BIND_JUMP:
					p.ToggleDucking(false);
					return new JumpingState();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 2:
		{
			switch(input)
			{
				case BIND_DOWN:
					return new NormalState();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
	}
	return nullptr;
}

void JumpingState::Enter(Player &p)
{
	p.Jump();
	//Play the jump sound
	//if (playSound) PlaySound("jump");
}

JumpingState::~JumpingState()
{
	pl->jumptime = 0;
	//pl->accel.y = 0;
}

EntityState* JumpingState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0:
		{
			switch(input)
			{
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 1:
		{
			switch(input)
			{
				case BIND_JUMP:
				{
					if(p.jumptime <= 0)
					{
						return new NormalState();
					}
					break;
				}
				case BIND_RIGHT:
					pl->SetDirection(DIRECTION_RIGHT);
					pl->Walk();
					break;
				case BIND_LEFT:
					pl->SetDirection(DIRECTION_LEFT);
					pl->Walk();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 2:
		{
			switch(input)
			{
				case BIND_JUMP:
					return new NormalState();
				case BIND_LEFT: case BIND_RIGHT:
					p.accel.x = 0;
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
	}
	return nullptr;
}

void HangingState::Enter(Player &p)
{
	PrintLog(LOG_DEBUG, "Switched to HANGING");
	pl->accel.y = 0;
	pl->accel.x = 0;
	pl->SetVelocity(0, 0);
}

HangingState::~HangingState()
{

}

EntityState* HangingState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0: // press
		{
			switch(input)
			{
				case BIND_RIGHT:
					//p.direction = true;
					break;
				case BIND_LEFT:
					//p.direction = false;
					break;
				case BIND_DOWN:
					p.lefthook = true;
					p.removeState(STATE_HANGING);
					return new NormalState();
					break;
				case BIND_JUMP:
					return new JumpingState();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 1:
		{
			switch(input)
			{
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
		case 2:
		{
			switch(input)
			{
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
	}
	return nullptr;
}

void SlidingState::Enter(Player &p)
{
	//p.ToggleSliding(true);
	PrintLog(LOG_INFO, "slide ON");
	p.hitbox->SetSize(10, 10);
	p.accel.x = 0;
	p.setState(STATE_SLIDING);
	//p.sprite->SetSpriteSize(30,10);
	//p.sprite->SetSpriteY(20);
	//p.sprite->SetSpriteOffset(-11, -32 + 1);
}

SlidingState::~SlidingState()
{
	//pl->ToggleSliding(false);
	PrintLog(LOG_INFO, "slide OFF");
	pl->hitbox->SetSize(10, 30);
	pl->hitbox->SetPos(pl->GetX(), pl->GetY() - 30);
	pl->accel.x = 0;
	pl->removeState(STATE_SLIDING);
	//pl->sprite->SetSpriteY(0);
	//pl->sprite->SetSpriteSize(10,30);
	//pl->sprite->SetSpriteOffset(-11, -32 + 1);
}

EntityState* SlidingState::HandleInput(Player &p, int input, int type)
{
	switch(type)
	{
		case 0:
		{
			switch(input)
			{
				case BIND_JUMP:
				{
					if(HasCeilingRightAbove(p))
						break;
					return new JumpingState();
					break;
				}
				case BIND_LEFT:
				{
					if(HasCeilingRightAbove(p))
					{
						if(p.GetVelocity().x == 0)
						{
							p.SetVelocity(-50, 0);
							p.direction = DIRECTION_LEFT;
							EntityState::HandleInput(p, input, type);
						}
						break;
					}
					if(p.GetVelocity().x > 0)
						return new NormalState();
					break;
				}
				case BIND_RIGHT:
				{
					if(HasCeilingRightAbove(p))
					{
						if(p.GetVelocity().x == 0)
						{
							p.SetVelocity(50, 0);
							p.direction = DIRECTION_RIGHT;
							EntityState::HandleInput(p, input, type);
						}
						break;
					}
					if(p.GetVelocity().x < 0)
						return new NormalState();
					break;
				}
				default:
					EntityState::HandleInput(p, input, type);
			}
		}
		case 1: // hold
		{
			switch(input)
			{
				case BIND_LEFT: case BIND_RIGHT: case BIND_DOWN:
				{
					if(HasCeilingRightAbove(p))
						break;
					if(!p.hasState(STATE_ONGROUND) || !IsOnIce(p) || p.GetVelocity().x == 0)
						return new NormalState();
					break;
				}
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 2: // unpress
		{
			switch(input)
			{
				case BIND_DOWN:
					if(HasCeilingRightAbove(p))
						break;
					return new NormalState();
					break;
				default:
					EntityState::HandleInput(p, input, type);
			}
			break;
		}
		case 3: // not holding anything specific
		{
			if(HasCeilingRightAbove(p))
				break;
			if(!IsBindPressed(BIND_DOWN))
				return new NormalState();
		}
	}
	return nullptr;
}

EntityState* SlidingState::HandleIdle(Player &p)
{
	return this->HandleInput(p, NULL, 3);
}