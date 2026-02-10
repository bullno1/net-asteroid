#include <bgame/scene.h>
#include <bgame/shader.h>
#include <cute.h>
#include "../assets.h"
#include "../gen/shaders/tiled.h"

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(test_background, TYPE, NAME)

SCENE_VAR(CF_Texture, texture)
SCENE_VAR(CF_Shader, shader)

static void
init(void) {
	const CF_Image* image = img_background_stars;
	CF_TextureParams tex_params = cf_texture_defaults(image->w, image->h);
	if (texture.id == 0) {
		texture = cf_make_texture(tex_params);
	}

	bgame_load_draw_shader(&shader, s_tiled_shd_bytecode);

	cf_texture_update(
		texture,
		image->pix, sizeof(image->pix[0]) * image->w * image->h
	);
}

static void
update(void) {
	cf_app_update(NULL);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);

	float width = cf_app_get_width();
	float height = cf_app_get_height();
	float half_width = width * 0.5f;
	float half_height = height * 0.5f;

	CF_Aabb screen = {
		.min = { -half_width, -half_height },
		.max = { +half_width, +half_height },
	};

	cf_draw_push_shader(shader);
	cf_draw_set_texture("tex_background", texture);
	cf_draw_set_uniform_v2("u_background_size", cf_v2(width, height));
	cf_draw_push_shape_aa(false);
	cf_draw_box(screen, 0.f, 0.f);
	cf_draw_pop_shape_aa();
	cf_draw_pop_shader();

	cf_app_draw_onto_screen(true);
}

static void
cleanup(void) {
	cf_destroy_shader(shader);
	shader.id = 0;
	cf_destroy_texture(texture);
	texture.id = 0;
}

BGAME_SCENE(test_background) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
