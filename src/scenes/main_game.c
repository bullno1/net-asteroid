#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <blog.h>
#include <cute.h>
#include <bent.h>
#include <bgame/asset.h>
#include "../ecs.h"
#include "../templates.h"
#include "../background.h"

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(test_spatial_hash, TYPE, NAME)

BGAME_DECLARE_SCENE_ALLOCATOR(main_game)

SCENE_VAR(bent_world_t*, world)
SCENE_VAR(background_t, background)
SCENE_VAR(bent_t, player_ship)

static CF_Rnd rnd = { 0 };

static void
init(void) {
	if (bent_init(&world, scene_allocator)) {
		rnd = cf_rnd_seed(CF_TICKS);

		for (int i = 0; i < 7; ++i) {
			create_asteroid(world, &rnd);
		}

		player_ship = create_player_ship(world);
	}

	init_background(&background, img_background_stars);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);
}

static void
cleanup(void) {
	cleanup_background(&background);
	bent_cleanup(&world);
}

static void
fixed_update(void* userdata) {
	ecs_update_fixed(world);
}

static void
update(void) {
	cf_app_update(fixed_update);
	ecs_update_variable(world);
	ecs_render(world);

	float width = cf_app_get_width();
	float height = cf_app_get_height();
	float half_width = width * 0.5f;
	float half_height = height * 0.5f;

	CF_V2 player_position = get_interpolated_transform(bent_get_comp_transform(world, player_ship)).p;
	player_position.y = -player_position.y;
	CF_Aabb screen = {
		.min = { -half_width, -half_height },
		.max = { +half_width, +half_height },
	};
	cf_draw_push_layer(DRAW_LAYER_BACKGROUND);
	draw_background(&background, screen, cf_mul(player_position, 0.3f), 1.f);
	cf_draw_pop_layer();

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main_game) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
