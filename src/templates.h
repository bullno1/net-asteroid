#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ecs.h"
#include <bgame/asset.h>
#include <bgame/asset/sprite.h>
#include <bgame/asset/collision_shape.h>
#include <cute.h>
#include "assets.h"

static inline bent_t
create_asteroid(
	bent_world_t* world,
	CF_Rnd* rnd
) {
	bent_t asteroid = bent_create(world);

	bent_add_comp_transform(world, asteroid, NULL);
	bent_add_comp_renderable(world, asteroid, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, asteroid, spr_asteroid_big_1);
	bent_add_comp_collider(world, asteroid, &(collider_t){
		.shape = shape_asteroid_big_1,
		.mask = COLLISION_BIT_ASTEROID,
		.group = COLLISION_BIT_ASTEROID,
	});
	bent_add_comp_linear_motion(world, asteroid, &(linear_motion_t){
		.rotation = cf_rnd_range_float(rnd, -2.f, 2.f),
		.velocity = {
			cf_rnd_range_float(rnd, -25.f, 25.f),
			cf_rnd_range_float(rnd, -25.f, 25.f),
		},
	});

	return asteroid;
}

static inline bent_t
create_player_ship(bent_world_t* world) {
	bent_t ent = bent_create(world);
	bent_add_comp_transform(world, ent, NULL);
	bent_add_comp_renderable(world, ent, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, ent, spr_player_ship);
	bent_add_comp_collider(world, ent, &(collider_t){
		.shape = shape_player_ship,
	});
	bent_add_comp_linear_motion(world, ent, NULL);
	bent_add_comp_ship(world, ent, NULL);
	bent_add_comp_player_ship(world, ent);
	bent_add_comp_ship_controller(world, ent);

	return ent;
}

static inline bent_t
create_friendly_projectile(bent_world_t* world) {
	bent_t ent = bent_create(world);
	bent_add_comp_transform(world, ent, NULL);
	bent_add_comp_renderable(world, ent, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, ent, spr_friendly_projectile);
	bent_add_comp_collider(world, ent, NULL);
	bent_add_comp_linear_motion(world, ent, NULL);

	return ent;
}

#endif
