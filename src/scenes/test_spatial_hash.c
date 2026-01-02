#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <blog.h>
#include <cute.h>
#include <bent.h>
#include <bgame/asset.h>
#include "../ecs.h"
#include "../templates.h"

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(test_spatial_hash, TYPE, NAME)

BGAME_DECLARE_SCENE_ALLOCATOR(test_spatial_hash)

SCENE_VAR(bent_world_t*, world)
SCENE_VAR(bgame_asset_bundle_t*, bundle)

static CF_Rnd rnd = { 0 };

static void
init(void) {
	bgame_asset_begin_load(&bundle);
	bgame_asset_end_load(bundle);

	if (bent_init(&world, scene_allocator)) {
		rnd = cf_rnd_seed(CF_TICKS);

		for (int i = 0; i < 10; ++i) {
			create_asteroid(world, bundle, &rnd);
		}

		create_player_ship(world, bundle, &rnd);
	}

	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);
}

static void
cleanup(void) {
	bent_cleanup(&world);
	bgame_asset_destroy_bundle(bundle);
	bundle = NULL;
}

static void
fixed_update(void* userdata) {
	ecs_update_fixed(world);
}

static void
update(void) {
	cf_app_update(fixed_update);

	bgame_asset_check_bundle(bundle);
	ecs_update_variable(world);
	ecs_render(world);

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(test_spatial_hash) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
