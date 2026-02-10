#include <bgame/reloadable.h>
#include <bgame/asset.h>
#include <bgame/shader.h>
#include <blog.h>
#include "gen/shaders/tiled.h"
#include "background.h"

BGAME_VAR(CF_Shader, tiled_shader) = { 0 };

void
init_background(background_t* background, CF_Image* image) {
	bgame_load_draw_shader(&tiled_shader, s_tiled_shd_bytecode);

	if (background->texture.id == 0) {
		CF_TextureParams params = cf_texture_defaults(image->w, image->h);
		background->texture = cf_make_texture(params);

		cf_texture_update(
			background->texture,
			image->pix, sizeof(image->pix[0]) * image->w * image->h
		);
		background->image_version = bgame_asset_version(image);
	}

	background->source_image = image;
	int image_version = bgame_asset_version(image);
	if (background->image_version != image_version) {
		cf_texture_update(
			background->texture,
			image->pix, sizeof(image->pix[0]) * image->w * image->h
		);
		background->image_version = bgame_asset_version(image);
	}
}

void
cleanup_background(background_t* background) {
	cf_destroy_texture(background->texture);
	background->texture.id = 0;
	background->image_version = 0;
}

void
draw_background(
	background_t* background,
	CF_Aabb box,
	CF_V2 offset,
	float scale
) {
	cf_draw_push_shader(tiled_shader);
	cf_draw_set_texture("tex_background", background->texture);
	cf_draw_set_uniform_v2("u_background_size", cf_extents(box));
	cf_draw_set_uniform_v2("u_offset", offset);
	cf_draw_set_uniform_float("u_scale", scale);
	cf_draw_push_shape_aa(1.0f);
	cf_draw_box(box, 0.f, 0.f);
	cf_draw_pop_shape_aa();
	cf_draw_pop_shader();
}
