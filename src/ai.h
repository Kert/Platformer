#ifndef _ai_h_
#define _ai_h_ 

#include <SDL.h>
#include <vector>
#include "globals.h"
#include "utils.h"

enum AI_TIMER_TYPE {
	AI_TIMER_REACH,
	AI_TIMER_SHOOT
};

class Creature;

class BaseAI
{
	protected:
		// internal generic vars
		bool distanceReached = false;

	protected:
		// Customizeable AI properties
		Creature *me;	
		Creature *target;
		std::vector<SDL_Point> targetPos;
		std::vector<double> timerTime;
		std::vector<double> timeToTrigger = {SecToTicks(0.1)};
		int distanceToLoss;
		int distanceToReach;
		int distanceToReachX = 200;
		int distanceToReachY = 200;
		double AIreactionTime = SecToTicks(0.01);
		double wanderChangeDirTime = SecToTicks(0.1);
		bool followTargetInAir = false;
		bool oneTimeToggle = false;

	public:
		void RunAI(double ticks);
		void SetDistanceToReachX(int x) { distanceToReachX = x; distanceToReach = 1000; };
		void SetDistanceToReachY(int y) { distanceToReachY = y; distanceToReach = 1000; };
		virtual void OnStateChange(CREATURE_STATES oldState, CREATURE_STATES newState) {};

	protected:
		BaseAI(Creature *c);
		virtual void Wander();
		virtual void TurnToTarget();
		virtual void OnTimerTimeup(int id) {};
		virtual void OnDistanceReached() {};
		virtual void OnDistanceLost() {};
		void Trigger(int id);
}; 

class AI_Chaser: public BaseAI
{
	private:
		bool chase;

	public:
		AI_Chaser(Creature *c) : BaseAI(c)
		{
			chase = false;
			distanceToReach = 200;
			distanceToLoss = 140;
			timeToTrigger = std::vector<double> { wanderChangeDirTime };
			timerTime = std::vector<double>{ 0 };
		};

	private:
		void OnDistanceReached();
		void OnDistanceLost();
		void OnTimerTimeup(int id);
};

class AI_ChaserJumper: public BaseAI
{
	private:
		bool chase;
		int distanceToJumpFrom;
		int threshold;

	public:
		AI_ChaserJumper(Creature *c) : BaseAI(c)
		{
			chase = false;
			distanceToJumpFrom = 60;
			threshold = 5;
			distanceToReach = 200;
			distanceToLoss = 500;
			timeToTrigger = std::vector<double> { SecToTicks(0.02) };
			timerTime = std::vector<double>{ 0 };
			followTargetInAir = false;
		};

	private:
		void OnDistanceReached();
		void OnDistanceLost();
		void OnTimerTimeup(int id);
};

class AI_Wanderer: public BaseAI
{
	public:
		AI_Wanderer(Creature *c) : BaseAI(c)
		{
			timeToTrigger = std::vector<double>{ SecToTicks(1) };
			timerTime = std::vector<double>{ 0 };
		};

	private:
		void OnTimerTimeup(int id);
};

class AI_Idle: public BaseAI
{
	public:
		AI_Idle(Creature *c) : BaseAI(c)
		{
			distanceToLoss = 30;
		};

	private:
		void OnDistanceReached();
};

class AI_HomingMissile: public BaseAI
{
	bool homing = false;
	public:
		AI_HomingMissile(Creature *c) : BaseAI(c)
		{
			distanceToReach = 130;
			timeToTrigger = std::vector<double> { SecToTicks(0.02), SecToTicks(2) };
			timerTime = std::vector<double>{ 0 ,0};
			followTargetInAir = true;
		};

	private:
		void OnDistanceReached();
		void OnTimerTimeup(int id);
};

class AI_Hypno: public BaseAI
{
	int radius = 40;
	SDL_Point center;
	int curDegree = 0;
	public:
		AI_Hypno(Creature *c) : BaseAI(c) {
			timeToTrigger = std::vector<double>{ 0 };
			timerTime = std::vector<double>{ 0 };
		};

	private:
		void OnTimerTimeup(int id);
};

class AI_Liner: public BaseAI
{
	public:
		AI_Liner(Creature *c) : BaseAI(c) {
			distanceToReach = 200;
		};

	private:
		void OnDistanceReached();
};

class AI_Sentinel: public BaseAI
{
	bool activated = false;
	public:
		AI_Sentinel(Creature *c) : BaseAI(c) {
			distanceToReach = 200;
			timeToTrigger = std::vector<double>{ SecToTicks(2) };
			timerTime = std::vector<double>{ 0 };
			oneTimeToggle = true;
		};

	private:
		void OnDistanceReached();
		void OnTimerTimeup(int id);
};

class AI_Anvil : public BaseAI
{
	public:
		AI_Anvil(Creature *c) : BaseAI(c) {
			distanceToReach = 1;
		};

	private:
		void OnDistanceReached();
};

class AI_Jumpingfire : public BaseAI
{
	bool activated = false;
	int startingY;
	public:
		AI_Jumpingfire(Creature *c) : BaseAI(c) {
			distanceToReach = 200;
			timeToTrigger = std::vector<double>{ 0 };
			timerTime = std::vector<double>{ 0 };
		};

	private:
		void OnDistanceReached();
		void OnTimerTimeup(int id);
};

class AI_GroundShockwaver : public BaseAI
{
	bool activated = false;
	int startingY;
public:
	AI_GroundShockwaver(Creature *c) : BaseAI(c) {
		distanceToReach = 200;
		timeToTrigger = std::vector<double>{ 0, SecToTicks(2) };
		timerTime = std::vector<double>{ 0 , 0 };
	};	

private:
	void OnDistanceReached();
	void OnTimerTimeup(int id);
	void OnStateChange(CREATURE_STATES oldState, CREATURE_STATES newState);
};

#endif
