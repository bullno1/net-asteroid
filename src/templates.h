#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "ecs.h"
#include <bgame/asset.h>
#include <bgame/asset/sprite.h>
#include <bgame/asset/collision_shape.h>
#include <cute.h>
#include <blog.h>
#include "assets.h"

static inline bent_t
create_asteroid(
	bent_world_t* world,
	CF_Rnd* rnd
) {
	bent_t asteroid = bent_create(world);

	CF_V2 pos = { 0 };
	float width = cf_app_get_width();
	float height = cf_app_get_height();
	float half_width = width * 0.5f;
	float half_height = height * 0.5f;
	int edge = cf_rnd_range_int(rnd, 0, 3);
	switch (edge) {
		case 0: // Top
			pos.x = cf_rnd_range_float(rnd, -half_width, half_width);
			pos.y = +half_height;
			break;
		case 1: // Bottom
			pos.x = cf_rnd_range_float(rnd, -half_width, half_width);
			pos.y = -half_height;
			break;
		case 2: // Left
			pos.x = -half_width;
			pos.y = cf_rnd_range_float(rnd, -half_height, half_height);
			break;
		case 3: // Right
			pos.x = +half_width;
			pos.y = cf_rnd_range_float(rnd, -half_height, half_height);
			break;
	}
	bent_add_comp_transform(world, asteroid, &(bgame_transform_t){
		.translation = pos,
		.scale = cf_v2(1.f, 1.f),
		.rotation = cf_rnd_range_float(rnd, 0.f, CF_PI * 2.f),
	});

	bent_add_comp_renderable(world, asteroid, &(renderable_t){ .layer = DRAW_LAYER_COMMON });
	bent_add_comp_sprite(world, asteroid, spr_asteroid_big_1);
	bent_add_comp_collider(world, asteroid, &(collider_t){
		.shape = shape_asteroid_big_1,
		.mask = COLLISION_BIT_ASTEROID,
		.group = COLLISION_BIT_ASTEROID,
	});

	float dir = cf_rnd_range_float(rnd, 0.f, CF_PI * 2.f);
	float speed = cf_rnd_range_float(rnd, 20.f, 40.f);
	CF_V2 velocity = { cf_cos_f(dir) * speed, cf_sin_f(dir) * speed };
	bent_add_comp_linear_motion(world, asteroid, &(linear_motion_t){
		.rotation = cf_rnd_range_float(rnd, -5.f, 5.f),
		.velocity = velocity,
	});

	bent_add_comp_asteroid(world, asteroid);
	bent_add_comp_wrap_around(world, asteroid);

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
	bent_add_comp_wrap_around(world, ent);
	bent_add_comp_ship_controller(world, ent);

	return ent;
}

static inline bent_t
create_friendly_projectile(bent_world_t* world, CF_V2 position, float rotation) {
	bent_t ent = bent_create(world);

	bent_add_comp_transform(world, ent, &(bgame_transform_t){
		.translation = position,
		.rotation = rotation,
		.scale = cf_v2(1.f, 1.f),
	});
	bent_add_comp_renderable(world, ent, &(renderable_t){ .layer = DRAW_LAYER_PROJECTILE });
	bent_add_comp_sprite(world, ent, spr_friendly_projectile);
	bent_add_comp_collider(world, ent, &(collider_t){
		.mask = COLLISION_BIT_ASTEROID,
	});
	const float SPEED = 400.f;
	bent_add_comp_linear_motion(world, ent, &(linear_motion_t){
		.velocity = {
			cf_sin_f(rotation) * SPEED,
			cf_cos_f(rotation) * SPEED,
		},
	});
	bent_add_comp_projectile(world, ent, &(projectile_t){
		.type = PROJECTILE_FRIENDLY,
	});
	bent_add_comp_offscreen_cull(world, ent);

	return ent;
}

#endif
