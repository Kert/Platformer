#ifndef _state_h_
#define _state_h_ 

#include "entities.h"

enum PLAYER_STATES
{
	PLAYER_STATE_ONGROUND,
	PLAYER_STATE_HANGING
};

class EntityState
{
public:
	Player *pl;
public:
	virtual EntityState* HandleInput(Player &p, int input, int type);
	virtual EntityState* HandleIdle(Player &p);
	virtual void Enter(Player &p) = 0;
	void Initialize(Player &p);
	virtual ~EntityState();
};

// On ground or in-air but NOT JUMPING/SLIDING e.t.c.
class NormalState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
	void Enter(Player &p);
};

class JumpingState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
public:
	void Enter(Player &p);
	~JumpingState();
};

class OnLadderState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
public:
	void Enter(Player &p);
	~OnLadderState();
};

class DuckingState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
public:
	void Enter(Player &p);
	~DuckingState();
};

class HangingState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
public:
	void Enter(Player &p);
	~HangingState();
};

class SlidingState : public EntityState
{
	virtual EntityState* HandleInput(Player &p, int input, int type);
	virtual EntityState* HandleIdle(Player &p);
public:
	void Enter(Player &p);
	~SlidingState();
};

#endif