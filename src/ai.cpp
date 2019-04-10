#include "ai.h"
#include "entities.h"
#include "level.h"
#include "state.h"
#include "tiles.h"
#include "utils.h"

RandomGenerator ai_rg;
extern Player *player;

BaseAI::BaseAI(Creature *c)
{
	for (int timers = 0; timers < (int)timeToTrigger.size(); timers++)
		timerTime.push_back(0);
	distanceReached = false;
	me = c;
	target = player;
}

void BaseAI::RunAI()
{
	for (int i = 0; i < (int)timeToTrigger.size(); i++)
	{
		timerTime[i]--;
		if (timerTime[i] < 0)
		{
			timerTime[i] = timeToTrigger[i];
			this->OnTimerTimeup(i);
		}
	}	

	if(me->GetDistanceToEntity(target) < distanceToReach && me->GetXDistanceToEntity(target) < distanceToReachX && me->GetYDistanceToEntity(target) < distanceToReachY)
	{
		if(!distanceReached)
		{
			PrintLog(LOG_SUPERDEBUG, "Distance reached");
			this->OnDistanceReached();
			distanceReached = true;
		}
	}
	else if(me->GetDistanceToEntity(target) > distanceToLoss)
	{
		if(oneTimeToggle)
			return;
		if(distanceReached)
		{
			PrintLog(LOG_SUPERDEBUG, "Distance lost");
			this->OnDistanceLost();
			distanceReached = false;
		}
	}
}

void BaseAI::Trigger(int id)
{
	timerTime[id] = 0;
}

void BaseAI::Wander()
{
	me->SwitchDirection();
	me->Walk();

 	int atX = (int)floor((me->GetX() - 1) / TILESIZE);
	if(me->direction == DIRECTION_LEFT)
		atX = (int)ceil((me->GetX() + me->hitbox->GetRect().w + 1) / TILESIZE);
	int atY = ConvertToTileCoord(me->GetY(), false);
	bool obstacled = (GetTileTypeAtTiledPos(atX, atY) == PHYSICS_BLOCK || GetTileTypeAtTiledPos(atX, atY) == PHYSICS_UNOCCUPIED);

	if(obstacled)
	{
		me->SwitchDirection();
		me->Walk();
	}
}

void BaseAI::TurnToTarget()
{
	if(me->GetX() < target->GetX())
		me->direction = DIRECTION_RIGHT;
	else
		me->direction = DIRECTION_LEFT;
}

void AI_Chaser::OnDistanceReached()
{
	chase = true;
	timeToTrigger[AI_TIMER_REACH] = AIreactionTime;
};

void AI_Chaser::OnDistanceLost()
{
	chase = false;
	timeToTrigger[AI_TIMER_REACH] = wanderChangeDirTime;
};

void AI_Chaser::OnTimerTimeup(int id)
{
	//PrintLog(LOG_SUPERDEBUG, "I'M CHASER");

	bool jump = true;

	if(chase)
	{
		if(me->state->Is(CREATURE_STATES::ONGROUND) || followTargetInAir)
		{
			if(target->GetX() < me->GetX())
				me->SetDirection(DIRECTION_LEFT);
			else
				me->SetDirection(DIRECTION_RIGHT);
		}
		me->Walk();
		int checktileX = ConvertToTileCoord(me->GetX() + me->hitbox->GetRect().w + 1, false);
		if(me->direction == DIRECTION_LEFT)
			checktileX = ConvertToTileCoord(me->GetX() - 1, false);
		int checktileY = ConvertToTileCoord(me->GetY(), false);

		bool obstacled = false;
		if(IsSolid(GetTileTypeAtTiledPos(checktileX, checktileY)) || IsSolid(GetTileTypeAtTiledPos(checktileX, checktileY - 1)))
			obstacled = true;

		if(jump && obstacled)
		{
			me->SetState(CREATURE_STATES::JUMPING);
		}
	}
	else
	{
		//this->Wander();
	}
}

void AI_ChaserJumper::OnDistanceReached()
{
	chase = true;
};

void AI_ChaserJumper::OnDistanceLost()
{
	chase = false;
};

void AI_ChaserJumper::OnTimerTimeup(int id)
{
	if(chase)
	{
		if(me->state->Is(CREATURE_STATES::ONGROUND) || followTargetInAir)
		{
			if(target->GetX() < me->GetX())
				me->SetDirection(DIRECTION_LEFT);
			else
				me->SetDirection(DIRECTION_RIGHT);
		}
		me->Walk();
		int checktileX = ConvertToTileCoord(me->GetX() + me->hitbox->GetRect().w + 1, false);
		if(me->direction == DIRECTION_LEFT)
			checktileX = ConvertToTileCoord(me->GetX() - 1, false);
		int checktileY = ConvertToTileCoord(me->GetY(), false);

		bool obstacled = false;
		if(IsSolid(GetTileTypeAtTiledPos(checktileX, checktileY)) || IsSolid(GetTileTypeAtTiledPos(checktileX, checktileY - 1)))
			obstacled = true;

		bool closeToJump = false;
		int distX = (int)me->GetXDistanceToEntity(target);
		if(distX < distanceToJumpFrom + threshold && distX > distanceToJumpFrom - threshold)
			closeToJump = true;
		if(obstacled || closeToJump)
		{
			me->SetState(CREATURE_STATES::JUMPING);
		}
	}
}

void AI_Wanderer::OnTimerTimeup(int id)
{
	PrintLog(LOG_SUPERDEBUG, "I'M WANDERER");

	this->Wander();
}

void AI_Idle::OnDistanceReached()
{
	me->SetState(CREATURE_STATES::JUMPING);
}

void AI_HomingMissile::OnDistanceReached()
{
	homing = true;
}

void AI_HomingMissile::OnTimerTimeup(int id)
{
	if(!homing)
		return;

	if (id == AI_TIMER_REACH)
	{
		if (me->GetX() < target->GetX())
			me->SetDirection(DIRECTION_RIGHT);
		else
			me->SetDirection(DIRECTION_LEFT);
		me->Walk();

		if (me->GetY() < target->GetY())
			me->MoveDown();
		else
			me->MoveUp();
	}
	if (id == AI_TIMER_SHOOT)
	{
		me->Shoot();
	}
}

void AI_Hypno::OnTimerTimeup(int id)
{
	curDegree+=2;
	if(curDegree > 360)
		curDegree = 0;

	double point_x = radius * sin(curDegree * M_PI / 180.) + target->GetX();
	double point_y = radius * cos(curDegree * M_PI / 180.) + target->GetY();
	me->SetPos(point_x, point_y);
}

void AI_Liner::OnDistanceReached()
{
	if(target->GetX() < me->GetX())
		me->SetDirection(DIRECTION_LEFT);
	else
		me->SetDirection(DIRECTION_RIGHT);
	me->Walk();
}

void AI_Sentinel::OnDistanceReached()
{
	activated = true;
	Trigger(AI_TIMER_REACH);
}

void AI_Sentinel::OnTimerTimeup(int id)
{
	if(activated)
	{
		this->TurnToTarget();
		me->Shoot();
	}
}

void AI_Anvil::OnDistanceReached()
{
	me->ignoreGravity = false;
}

void AI_Jumpingfire::OnDistanceReached()
{
	activated = true;
	Trigger(AI_TIMER_REACH);
}

void AI_Jumpingfire::OnTimerTimeup(int id)
{
	if (activated)
	{
		if (me->GetY() > startingY - 1) {
			if (me->ignoreGravity == false) // blatant HACK
			{
				me->status = STATUS_DYING;
				me->statusTimer = 1;
			}
			me->ignoreGravity = false;
			me->SetVelocity(0, -1000);
		}
	}
	else
	{
		startingY = (int)me->GetY();
	}
}

void AI_GroundShockwaver::OnTimerTimeup(int id)
{
	if(!activated)
		return;

	if(id == AI_TIMER_SHOOT)
	{
		me->SetState(CREATURE_STATES::JUMPING);
	}
}

void AI_GroundShockwaver::OnStateChange(CREATURE_STATES oldState, CREATURE_STATES newState)
{
	if(!activated)
		return;

	if(newState == CREATURE_STATES::ONGROUND)
	{
		DIRECTIONS oldDirection = me->direction;
		me->direction = DIRECTION_LEFT;
		me->Shoot();
		me->direction = DIRECTION_RIGHT;
		me->Shoot();
		me->direction = oldDirection;
	}
}

void AI_GroundShockwaver::OnDistanceReached()
{
	if(!activated)
	{
		me->weapon = WEAPON_GROUNDSHOCKWAVE;
		activated = true;
	}
}