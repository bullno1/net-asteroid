#include "../ecs.h"
#include <bgame/draw_list.h>
#include <cute.h>
#include <bminmax.h>

static void
animate_sprite(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	for (bent_index_t i = 0; i < num_entities; ++i) {
		cf_sprite_update(bent_get_comp_sprite(world, entities[i]));
	}
}

BENT_DEFINE_SYS(sys_animate_sprite) = {
	.update_mask = UPDATE_MASK_VAR,
	.update = animate_sprite,
	.require = BENT_COMP_LIST(&comp_sprite),
};

static void
render_sprite(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	bgame_draw_list_t* draw_list = bgame_alloc_draw_list(num_entities);
	for (bent_index_t i = 0; i < num_entities; ++i) {
		draw_list->items[i].entity_id = entities[i];
		draw_list->items[i].sort_key = bent_get_comp_renderable(world, entities[i])->layer;
	}

	bgame_sort_draw_list(draw_list);

	float width = cf_app_get_width();
	float height = cf_app_get_height();
	float min_x = -width * 0.5f;
	float max_x =  width * 0.5f;
	float min_y = -height * 0.5f;
	float max_y =  height * 0.5f;

	int itr = 0;
	for (draw_layer_t layer = 0; layer < DRAW_LAYER_COUNT; ++layer) {
		cf_draw_push_layer(layer);
		bgame_draw_item_t* draw_item;
		while ((draw_item = bgame_next_draw_item(draw_list, &itr, layer)) != NULL) {
			CF_Sprite* sprite = bent_get_comp_sprite(world, draw_item->entity_id);
			transform_t* transform_data = bent_get_comp_transform(world, draw_item->entity_id);
			CF_M3x2 transform = get_interpolated_transform(transform_data);

			cf_draw_push();
			cf_draw_transform(transform);
			cf_draw_sprite(sprite);
			cf_draw_pop();

			int mirror_x = 0;
			int mirror_y = 0;
			float x = transform_data->current.translation.x;
			float y = transform_data->current.translation.y;
			float radius = BMAX(sprite->w, sprite->h);

			if (x - radius < min_x) mirror_x = +1;
			if (x + radius > max_x) mirror_x = -1;

			if (y - radius < min_y) mirror_y = +1;
			if (y + radius > max_y) mirror_y = -1;

			if (mirror_x) {
				CF_M3x2 mirrored = transform;
				mirrored.p.x = transform.p.x + mirror_x * width;

				cf_draw_push();
				cf_draw_transform(mirrored);
				cf_draw_sprite(sprite);
				cf_draw_pop();
			}

			if (mirror_y) {
				CF_M3x2 mirrored = transform;
				mirrored.p.y = transform.p.y + mirror_y * height;

				cf_draw_push();
				cf_draw_transform(mirrored);
				cf_draw_sprite(sprite);
				cf_draw_pop();
			}

			/* Corner mirror */
			if (mirror_x && mirror_y) {
				CF_M3x2 mirrored = transform;
				mirrored.p.x = transform.p.x + mirror_x * width;
				mirrored.p.y = transform.p.y + mirror_y * height;

				cf_draw_push();
				cf_draw_transform(mirrored);
				cf_draw_sprite(sprite);
				cf_draw_pop();
			}

		}
		cf_draw_pop_layer();
	}
}

BENT_DEFINE_SYS(sys_render_sprite) = {
	.update_mask = UPDATE_MASK_RENDER,
	.update = render_sprite,
	.require = BENT_COMP_LIST(&comp_transform, &comp_sprite, &comp_renderable),
};
