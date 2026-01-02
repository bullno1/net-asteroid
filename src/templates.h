#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ecs.h"
#include <bgame/asset.h>
#include <bgame/asset/sprite.h>
#include <bgame/asset/collision_shape.h>
#include <cute.h>

static inline bent_t
create_asteroid(
	bent_world_t* world,
	bgame_asset_bundle_t* bundle,
	CF_Rnd* rnd
) {
	bent_t asteroid = bent_create(world);

	bent_add_comp_transform(world, asteroid, NULL);
	bent_add_comp_renderable(world, asteroid, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, asteroid, bgame_load_sprite(bundle, "/assets/sprites/asteroid_big1.png"));
	bent_add_comp_collider(world, asteroid, &(collider_t){
		.shape = bgame_load_collision_shape(bundle, "/assets/shapes/meteor-big1.json"),
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
create_player_ship(
	bent_world_t* world,
	bgame_asset_bundle_t* bundle,
	CF_Rnd* rnd
) {
	bent_t ent = bent_create(world);
	bent_add_comp_transform(world, ent, NULL);
	bent_add_comp_renderable(world, ent, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, ent, bgame_load_sprite(bundle, "/assets/sprites/player_ship_blue.ase"));
	bent_add_comp_collider(world, ent, &(collider_t){
		.shape = bgame_load_collision_shape(bundle, "/assets/shapes/player-ship.json"),
	});
	bent_add_comp_linear_motion(world, ent, NULL);
	bent_add_comp_player_ship(world, ent);
	bent_add_comp_ship_controller(world, ent);

	return ent;
}

#endif
