#include "ecs.h"

static void
comp_transform_init(void* data, void* arg) {
	transform_t* transform = data;
	bgame_transform_t* initial = arg;
	if (initial == NULL) {
		transform->previous = transform->current = bgame_transform_identity();
	} else {
		transform->previous = transform->current = *initial;
	}
}

BENT_DEFINE_COMP(comp_transform) = {
	.size = sizeof(transform_t),
	.init = comp_transform_init,
};

BENT_DEFINE_POD_COMP(comp_sprite, CF_Sprite)
BENT_DEFINE_POD_COMP(comp_collider, collider_t)
BENT_DEFINE_POD_COMP(comp_renderable, renderable_t)
BENT_DEFINE_POD_COMP(comp_linear_motion, linear_motion_t)
BENT_DEFINE_TAG_COMP(comp_player_ship)
BENT_DEFINE_TAG_COMP(comp_ship_controller)
BENT_DEFINE_POD_COMP(comp_ship, ship_t)

typedef struct {
	bgame_asset_bundle_t* bundle;
	bool loading;
} asset_manager_t;

static void
asset_manager_init(void* userdata, bent_world_t* world) {
	asset_manager_t* sys = userdata;
	bgame_asset_init(&sys->bundle, bent_memctx(world));
	sys->loading = true;
}

static void
asset_manager_cleanup(void* userdata, bent_world_t* world) {
	asset_manager_t* sys = userdata;
	bgame_asset_cleanup(&sys->bundle);
}

static void
asset_manager_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	asset_manager_t* sys = userdata;
	bgame_asset_check_bundle(sys->bundle);
}

BENT_DEFINE_SYS(sys_asset_manager) = {
	.size = sizeof(asset_manager_t),
	.init = asset_manager_init,
	.allow_reinit = true,
	.cleanup = asset_manager_cleanup,
	.update_mask = UPDATE_MASK_FIXED,
	.update = asset_manager_update,
};

bgame_asset_bundle_t*
ecs_get_asset_bundle(bent_world_t* world) {
	asset_manager_t* sys = bent_get_sys_data(world, sys_asset_manager);
	return sys->bundle;
}
