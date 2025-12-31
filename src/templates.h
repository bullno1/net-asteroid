#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ecs.h"
#include <bgame/asset.h>
#include <bgame/asset/sprite.h>
#include "asteroid.h"
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
	bent_add_comp_collider(world, asteroid, NULL);
	bent_add_comp_asteroid(world, asteroid, &(asteroid_t){
		.rotation = cf_rnd_range_float(rnd, -2.f, 2.f),
		.velocity = {
			cf_rnd_range_float(rnd, -25.f, 25.f),
			cf_rnd_range_float(rnd, -25.f, 25.f),
		},
	});

	return asteroid;
}

#endif
