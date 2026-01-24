#include "../ecs.h"
#include "../slopsync.h"
#include <bgame/collision.h>
#include <cute.h>

static void
offscreen_cull_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	CF_Aabb screen = cf_make_aabb_pos_w_h(
		cf_v2(0.f, 0.f),
		cf_app_get_width(), cf_app_get_height()
	);
	BENT_FOREACH_ENTITY(ent, entities) {
		transform_t* transform = bent_get_comp_transform(world, ent);
		collider_t* collider = bent_get_comp_collider(world, ent);

		CF_M3x2 aabb_transform = cf_make_transform_TSR(
			transform->current.translation,
			transform->current.scale,
			transform->current.rotation
		);
		CF_Aabb aabb = bgame_make_aabb_from_shape(collider->shape, aabb_transform);
		if (!cf_aabb_to_aabb(screen, aabb)) {
			bent_destroy(world, ent);
		}
	}
}

BENT_DEFINE_SYS(sys_offscreen_cull) = {
	.update_mask = UPDATE_MASK_FIXED_POST,
	.update = offscreen_cull_update,
	.require = BENT_COMP_LIST(
		&comp_transform,
		&comp_offscreen_cull,
		&comp_collider
	),
	.exclude = BENT_COMP_LIST(
		&comp_slopsync_remote
	),
};
