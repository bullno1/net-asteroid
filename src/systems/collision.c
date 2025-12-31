#include "collision.h"
#include <bgame/allocator.h>
#include <bgame/allocator/frame.h>
#include <cute.h>

static CF_Aabb
transform_aabb(CF_Aabb aabb, CF_M3x2 matrix) {
	CF_V2 corners[4];
	cf_aabb_verts(corners, aabb);

	// Transform each corner
	for (int i = 0; i < 4; i++) {
		corners[i] = cf_mul(matrix, corners[i]);
	}

	return cf_make_aabb_verts(corners, CF_ARRAY_SIZE(corners));;
}

static void
collision_init(void* userdata, bent_world_t* world) {
	spatial_hash_init(userdata, bgame_default_allocator, bgame_arena_pool, 128);
}

static void
collision_cleanup(void* userdata, bent_world_t* world) {
	spatial_hash_cleanup(userdata);
}

static void
collision_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	spatial_hash_t* sh = userdata;
	if (update_mask == UPDATE_MASK_FIXED_PRE) {
		spatial_hash_clear(sh);
		for (bent_index_t i = 0; i < num_entities; ++i) {
			bent_t ent = entities[i];
			bgame_transform_t* transform = &bent_get_comp_transform(world, entities[i])->current;
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			CF_Sprite* sprite = bent_get_comp_sprite(world, entities[i]);

			CF_Aabb aabb = cf_make_aabb_center_half_extents(cf_v2(0.f, 0.f), cf_v2(sprite->w * 0.5f, sprite->h * 0.5f));
			CF_Aabb transformed_aabb = transform_aabb(aabb, mat);

			uint64_t id = (uint64_t)ent.index << 32 | (uint64_t)ent.gen;
			spatial_hash_insert(sh, transformed_aabb, id);
		}
	}

	if (update_mask == UPDATE_MASK_RENDER_POST) {
		cf_draw_push_layer(DRAW_LAYER_DEBUG);
		for (bent_index_t i = 0; i < num_entities; ++i) {
			bgame_transform_t* transform = &bent_get_comp_transform(world, entities[i])->current;
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			CF_Sprite* sprite = bent_get_comp_sprite(world, entities[i]);

			CF_Aabb aabb = cf_make_aabb_center_half_extents(cf_v2(0.f, 0.f), cf_v2(sprite->w * 0.5f, sprite->h * 0.5f));
			CF_Aabb transformed_aabb = transform_aabb(aabb, mat);

			cf_draw_box(transformed_aabb, 0.5f, 0.5f);
			cf_draw_push();
			cf_draw_transform(mat);
			cf_draw_box(aabb, 0.5f, 0.5f);
			cf_draw_pop();
		}

		for (bhash_index_t i = 0; i < bhash_len(&sh->cells); ++i) {
			spatial_hash_coord_t coord = sh->cells.keys[i];
			int size = 0;
			spatial_hash_cell_entry_t* entry = sh->cells.values[i];
			for (spatial_hash_cell_entry_t* itr = entry; itr != NULL; itr = itr->next) {
				size += itr->len;
			}

			CF_Aabb cell_box = spatial_hash_get_cell_coord(sh, coord);
			cf_draw_push_color(size > 1 ? cf_color_orange() : cf_color_green());
			cf_draw_box(cell_box, 0.5f, 0.5f);

			if (size > 1) {
				const char* num_items = bgame_fmt("%d", size);
				cf_draw_text(num_items, cf_center(cell_box), -1);
			}

			cf_draw_pop_color();
		}

		cf_draw_pop_layer();
	}
}

BENT_DEFINE_SYS(sys_collision) = {
	.size = sizeof(spatial_hash_t),
	.init = collision_init,
	.allow_reinit = true,
	.cleanup = collision_cleanup,
	.update_mask = UPDATE_MASK_FIXED_PRE | UPDATE_MASK_RENDER_POST,
	.update = collision_update,
	// We should use collider shape instead but for now, sprite is good enough
	.require = BENT_COMP_LIST(&comp_transform, &comp_sprite),
};
