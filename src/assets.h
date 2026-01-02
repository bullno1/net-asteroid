#ifndef ASSETS_H
#define ASSETS_H

#include <bgame/asset/sprite.h>
#include <bgame/asset/collision_shape.h>

extern struct CF_Sprite* spr_player_ship;
extern struct CF_Sprite* spr_asteroid_big_1;
extern struct CF_Sprite* spr_exhaust;
extern struct CF_Sprite* spr_friendly_projectile;

extern bgame_collision_shape_t* shape_asteroid_big_1;
extern bgame_collision_shape_t* shape_player_ship;

#endif
