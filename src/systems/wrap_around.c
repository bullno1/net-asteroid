#include "../ecs.h"
#include <cute_app.h>
#include <blog.h>

static inline float wrapf(float v, float min, float max) {
    float range = max - min;
    return min + fmodf(fmodf(v - min, range) + range, range);
}

static void
wrap_around(float* value, float bound) {
	float half_bound = bound * 0.5f;
	*value = wrapf(*value, -half_bound, half_bound);
}

static void
wrap_around_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	for (bent_index_t i = 0; i < num_entities; ++i) {
		transform_t* transform = bent_get_comp_transform(world, entities[i]);

		CF_V2 pos = transform->current.translation;
		wrap_around(&transform->current.translation.x, cf_app_get_width());
		wrap_around(&transform->current.translation.y, cf_app_get_height());

		if (cf_len(cf_sub(pos, transform->current.translation)) > 0.1f) {
			snap_transform(transform);
		}
	}
}

BENT_DEFINE_SYS(sys_wraparound) = {
	.update_mask = UPDATE_MASK_FIXED_POST,
	.update = wrap_around_update,
	.require = BENT_COMP_LIST(&comp_transform),
};
