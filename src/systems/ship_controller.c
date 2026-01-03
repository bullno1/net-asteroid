#include "../ecs.h"
#include <cute.h>
#include "../templates.h"

static void
ship_controller_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	if (num_entities != 1) { return; }

	bent_t ent = entities[0];
	bgame_transform_t* transform = &bent_get_comp_transform(world, ent)->current;
	linear_motion_t* linear_motion = bent_get_comp_linear_motion(world, ent);
	ship_t* ship = bent_get_comp_ship(world, ent);
	sprite_t* sprite = bent_get_comp_sprite(world, ent);

	const float TURN_RATE = 60.0f;
	if (cf_key_down(CF_KEY_A)) {
		linear_motion->rotation = -TURN_RATE * CF_DELTA_TIME_FIXED;
	} else if (cf_key_down(CF_KEY_D)) {
		linear_motion->rotation =  TURN_RATE * CF_DELTA_TIME_FIXED;
	} else {
		linear_motion->rotation =  0.f;
	}
	ship->turning = linear_motion->rotation;

	const float MAX_SPEED = 210.f;
	const float THRUST = 170.f;
	const float BRAKE_FACTOR = 0.75f;  // Brake is weaker than manual counter thrust
	ship->thrusting = false;
	ship->braking = false;
	if (cf_key_down(CF_KEY_W)) {
		CF_V2 thrust_vector = cf_mul(
			cf_v2(cf_sin_f(transform->rotation), cf_cos_f(transform->rotation)),
			THRUST * CF_DELTA_TIME_FIXED
		);
		CF_V2 new_velocity = cf_add(linear_motion->velocity, thrust_vector);
		float new_speed = cf_len(new_velocity);
		linear_motion->velocity = cf_mul(cf_norm(new_velocity), cf_min(new_speed, MAX_SPEED));
		ship->thrusting = true;
	} else if (cf_key_down(CF_KEY_S)) {
		float speed = cf_len(linear_motion->velocity);
		if (speed < 1e-6) {
			linear_motion->velocity.x = 0.f;
			linear_motion->velocity.y = 0.f;
		} else {
			const float counterthrust = -1.f * cf_min(THRUST * BRAKE_FACTOR * CF_DELTA_TIME_FIXED, speed);
			CF_V2 thrust_vector = cf_mul(cf_norm(linear_motion->velocity), counterthrust);
			linear_motion->velocity = cf_add(linear_motion->velocity, thrust_vector);
			ship->braking = true;
		}
	}

	const float ROUNDS_PER_SEC = 2.f;
	const float fire_interval_s = 1.f / ROUNDS_PER_SEC;
	if (cf_key_down(CF_KEY_SPACE)) {
		ship->firing = true;

		if (CF_SECONDS - ship->last_fire_timestamp_s >= fire_interval_s) {
			ship->last_fire_timestamp_s = CF_SECONDS;

			CF_M3x2 ship_transform = cf_make_transform_TSR(
				transform->translation, transform->scale, transform->rotation
			);
			CF_V2 nozzle = cf_center(cf_sprite_get_slice(sprite->asset, "nozzle"));
			CF_V2 projectile_pos = cf_mul(ship_transform, nozzle);
			create_friendly_projectile(world, projectile_pos, transform->rotation);
		} else {
			ship->charge_progress = (CF_SECONDS - ship->last_fire_timestamp_s) / fire_interval_s;
		}
	} else {
		ship->firing = false;
	}
}

BENT_DEFINE_SYS(sys_ship_controller) = {
	.update_mask = UPDATE_MASK_FIXED,
	.update = ship_controller_update,
	.require = BENT_COMP_LIST(
		&comp_ship,
		&comp_sprite,
		&comp_ship_controller,
		&comp_linear_motion,
		&comp_transform
	),
};
