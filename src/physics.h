#ifndef _physics_h_
#define _physics_h_ 

#include <SDL.h>

#include "entities.h"
#include "globals.h"

void ApplyForces(Creature &c, double ticks);
void DetectAndResolveMapCollisions(Creature &p);
void DetectAndResolveEntityCollisions(Creature &p);
bool HasCollisionWithEntity(Creature &p, Machinery &m);

void ApplyPhysics(Machinery &d, double ticks);
void ApplyPhysics(Creature &c, double ticks);
bool ApplyPhysics(Bullet &b, double ticks);
bool ApplyPhysics(Lightning &l, double ticks);

void ApplyKnockback(Creature &p, DIRECTIONS dir);
void ApplyKnockback(Creature &p, Creature &e);

bool UpdateStatus(Effect &e, double deltaTicks);
void UpdateStatus(Creature &e, double ticks);
bool UpdateStatus(Pickup &p, double deltaTicks);

void OnHitboxCollision(Creature &p, Creature &e, double ticks);
void OnHitboxCollision(Creature &c, Pickup &p, double ticks);

std::pair<double, double> GetAngleSinCos(DynamicEntity &shooter);
std::pair<std::vector<Creature*>, std::vector<Machinery*>> CheckForCollision(Bullet *entity);
void ProcessShot(WEAPONS weapon, Creature &shooter);
bool IsInDeathZone(Creature &c);
bool IsOnIce(Creature &c);
bool IsOnPlatform(Creature &c);
bool IsInRain(DynamicEntity &c);
bool IsSolid(PHYSICS_TYPES type);
bool HasCeilingRightAbove(DynamicEntity &c);

#endif
