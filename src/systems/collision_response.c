#include "collision.h"
#include <blog.h>

BENT_DECLARE_SYS(sys_collision_response)

static void
collision_response_on_collision(
	void* userdata, bent_world_t* world,
	bent_t entity, const collision_event_t* event
) {
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

static void
collision_response_post_init(void* userdata, bent_world_t* world) {
	register_collision_callback(world, sys_collision_response, collision_response_on_collision);
}

BENT_DEFINE_SYS(sys_collision_response) = {
	.post_init = collision_response_post_init,
	.require = BENT_COMP_LIST(&comp_linear_motion, &comp_collider),
};
