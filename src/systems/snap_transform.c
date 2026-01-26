#include "../ecs.h"
#include "../slopsync.h"

static void
sys_snap_transform_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	for (bent_index_t i = 0; i < num_entities; ++i) {
		snap_transform(bent_get_comp_transform(world, entities[i]));
	}
}

BENT_DEFINE_SYS(sys_snap_transform) = {
	.update_mask = UPDATE_MASK_FIXED_PRE,
	.require = BENT_COMP_LIST(&comp_transform),
	.update = sys_snap_transform_update,
};
