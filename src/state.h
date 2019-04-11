#ifndef _state_h_
#define _state_h_ 

#include "entities.h"

class CreatureState
{
public:
	Creature *cr;
	CREATURE_STATES state;

public:
	virtual CreatureState* HandleInput(int input, int type);
	virtual CreatureState* HandleIdle();
	CREATURE_STATES GetState() { return state; };
	bool Is(CREATURE_STATES state) { return this->state == state; };
	CreatureState(Creature *cr);
	virtual ~CreatureState();
};

// On ground standing or walking
class OnGroundState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
public:
	OnGroundState(Creature *cr);
};

class InAirState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
public:
	InAirState(Creature *cr);
};

class JumpingState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
public:
	JumpingState(Creature *cr);
	~JumpingState();
};

class DuckingState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
public:
	DuckingState(Creature *cr);
	~DuckingState();
};

class HangingState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
public:
	HangingState(Creature *cr);
	~HangingState();
};

class SlidingState : public CreatureState
{
	virtual CreatureState* HandleInput(int input, int type);
	virtual CreatureState* HandleIdle();
public:
	SlidingState(Creature *cr);
	~SlidingState();
};

#endif