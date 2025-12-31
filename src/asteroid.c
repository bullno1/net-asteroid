#include "asteroid.h"
#include <cute.h>

BENT_DEFINE_POD_COMP(comp_asteroid, asteroid_t)

static void
asteroid_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	for (bent_index_t i = 0; i < num_entities; ++i) {
		asteroid_t* asteroid = bent_get_comp_asteroid(world, entities[i]);
		bgame_transform_t* transform = &bent_get_comp_transform(world, entities[i])->current;

		transform->translation = cf_add(transform->translation, cf_mul(asteroid->velocity, CF_DELTA_TIME_FIXED));
		transform->rotation = transform->rotation + asteroid->rotation * CF_DELTA_TIME_FIXED;
	}
}

BENT_DEFINE_SYS(sys_asteroid) = {
	.update_mask = UPDATE_MASK_FIXED,
	.update = asteroid_update,
	.require = BENT_COMP_LIST(&comp_asteroid, &comp_transform),
};
