#ifndef _physics_h_
#define _physics_h_ 

#include <SDL.h>

#include "entities.h"
#include "globals.h"

#define PHYSICS_SPEED_FACTOR 1.0f
#define PHYSICS_SPEED 0.001 * PHYSICS_SPEED_FACTOR

#define GRAVITY 1500
#define AIR_RESISTANCE 500
#define FRICTION 9999

void ApplyForces(Creature &c, Uint32 deltaTicks);
void DetectAndResolveMapCollisions(Creature &p);
void DetectAndResolveEntityCollisions(Creature &p);
bool HasCollisionWithEntity(Creature &p, Machinery &m, SDL_Rect &result);

void ApplyPhysics(Machinery &d, Uint32 deltaTicks);
void ApplyPhysics(Creature &c, Uint32 deltaTicks);
bool ApplyPhysics(Bullet &b, Uint32 deltaTicks);
bool ApplyPhysics(Lightning &l, Uint32 deltaTicks);

void ApplyKnockback(Creature &p, Creature &e);

bool UpdateStatus(Effect &e, Uint32 deltaTicks);
void UpdateStatus(Creature &e, Uint32 deltaTicks);
bool UpdateStatus(Pickup &p, Uint32 deltaTicks);

void OnHitboxCollision(Creature &p, Creature &e, Uint32 deltaTicks);
void OnHitboxCollision(Creature &c, Pickup &p, Uint32 deltaTicks);

std::pair<double, double> GetAngleSinCos(DynamicEntity &shooter);
std::pair<Creature*, Machinery*> CheckForCollision(Bullet *entity);
void ProcessShot(WEAPONS weapon, Creature &shooter);
bool IsInDeathZone(Creature &c);
bool IsOnIce(Creature &c);
bool IsInRain(DynamicEntity &c);
bool HasCeilingRightAbove(DynamicEntity &c);

#endif
