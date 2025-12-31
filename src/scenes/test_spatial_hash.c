#include <bgame/scene.h>
#include <bgame/reloadable.h>
#include <cute_app.h>

static void
init(void) {
}

static void
update(void) {
	cf_app_update(NULL);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);
	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(test_spatial_hash) = {
	.init = init,
	.update = update,
};
