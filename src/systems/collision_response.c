#include "collision.h"
#include <blog.h>

#define COLLISION_RESPONSE_RULE(NAME) \
	BENT_DECLARE_SYS(sys_collision_response_##NAME) \
	static void \
	sys_collision_response_##NAME##_post_init(void* userdata, bent_world_t* world) { \
		register_collision_callback(world, sys_collision_response_##NAME, NAME.callback); \
	} \
	BENT_DEFINE_SYS(sys_collision_response_##NAME) = { \
		.post_init = sys_collision_response_##NAME##_post_init, \
		.require = NAME.require, \
		.exclude = NAME.exclude, \
	};

typedef struct {
	collision_callback_fn_t callback;
	bent_comp_reg_t** require;
	bent_comp_reg_t** exclude;
} collision_response_rule_t;

static void
process_asteroid(
	void* userdata, bent_world_t* world,
	bent_t entity, const collision_event_t* event
) {
	bent_t other_entity = collision_get_other_entity(entity, event);
	if (bent_has(world, other_entity, comp_asteroid)) {
		// Since the direciton of CF_Manifold.n points from a to b, we need to
		// invert it to separate the two objects.
		// First, we need to find out whether this entity is a or b.
		float multiplier = bent_equal(entity, event->pair.a)
			? -1.f   // This entity is a, invert
			:  1.f;  // This entity is b, follow
		linear_motion_t* motion = bent_get_comp_linear_motion(world, entity);
		float magnitude = cf_len(motion->velocity);
		motion->velocity = cf_mul(event->manifold.n, magnitude * multiplier);
	}

	if (bent_has(world, other_entity, comp_projectile)) {
		bent_destroy(world, entity);
	}
}

static const collision_response_rule_t rule_asteroid = {
	.callback = process_asteroid,
	.require = BENT_COMP_LIST(&comp_asteroid),
};
COLLISION_RESPONSE_RULE(rule_asteroid)

static void
process_projectile(
	void* userdata, bent_world_t* world,
	bent_t entity, const collision_event_t* event
) {
	bent_destroy(world, entity);
}

static const collision_response_rule_t rule_projectile = {
	.callback = process_projectile,
	.require = BENT_COMP_LIST(&comp_projectile),
};
COLLISION_RESPONSE_RULE(rule_projectile)
