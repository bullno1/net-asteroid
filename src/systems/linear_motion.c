#include "../ecs.h"

static void
linear_motion_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	for (bent_index_t i = 0; i < num_entities; ++i) {
		linear_motion_t* linear_motion = bent_get_comp_linear_motion(world, entities[i]);
		bgame_transform_t* transform = &bent_get_comp_transform(world, entities[i])->current;

		transform->translation = cf_add(transform->translation, cf_mul(linear_motion->velocity, CF_DELTA_TIME_FIXED));
		transform->rotation = transform->rotation + linear_motion->rotation * CF_DELTA_TIME_FIXED;
	}
}

BENT_DEFINE_SYS(sys_linear_motion) = {
	.update_mask = UPDATE_MASK_FIXED,
	.update = linear_motion_update,
	.require = BENT_COMP_LIST(&comp_linear_motion, &comp_transform),
};
