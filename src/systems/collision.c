#include "collision.h"
#include <bgame/allocator.h>
#include <bgame/allocator/frame.h>
#include <cute.h>

static CF_Aabb
make_aabb(const bgame_collision_shape_t* shape, CF_M3x2 transform) {
	switch (shape->type) {
		case CF_SHAPE_TYPE_NONE:
			return (CF_Aabb){ 0 };
		case CF_SHAPE_TYPE_AABB: {
			CF_V2 corners[4];
			cf_aabb_verts(corners, shape->aabb);

			// Transform each corner
			for (int i = 0; i < 4; i++) {
				corners[i] = cf_mul(transform, corners[i]);
			}
			return cf_make_aabb_verts(corners, CF_ARRAY_SIZE(corners));;
		} break;
		case CF_SHAPE_TYPE_POLY: {
			CF_V2 verts[CF_POLY_MAX_VERTS];

			for (int i = 0; i < shape->poly.count; i++) {
				verts[i] = cf_mul(transform, shape->poly.verts[i]);
			}
			return cf_make_aabb_verts(verts, shape->poly.count);
		} break;
		case CF_SHAPE_TYPE_CIRCLE: {
			// Transform the center of the circle
			CF_V2 center_transformed = cf_mul(transform, shape->circle.p);

			// The radius gets scaled by the matrix
			// Get the scale from the matrix by transforming a unit vector
			CF_V2 scale_x = cf_sub(cf_mul(transform, cf_v2(1, 0)), cf_mul(transform, cf_v2(0, 0)));
			CF_V2 scale_y = cf_sub(cf_mul(transform, cf_v2(0, 1)), cf_mul(transform, cf_v2(0, 0)));
			float scale = cf_max(cf_len(scale_x), cf_len(scale_y));
			float transformed_radius = shape->capsule.r * scale;

			// Create AABB centered on the transformed circle center
			float min_x = center_transformed.x - transformed_radius;
			float max_x = center_transformed.x + transformed_radius;
			float min_y = center_transformed.y - transformed_radius;
			float max_y = center_transformed.y + transformed_radius;

			return cf_make_aabb(cf_v2(min_x, min_y), cf_v2(max_x, max_y));
		} break;
		case CF_SHAPE_TYPE_CAPSULE: {
			// Transform the two endpoints of the capsule
			CF_V2 a_transformed = cf_mul(transform, shape->capsule.a);
			CF_V2 b_transformed = cf_mul(transform, shape->capsule.b);

			// The radius also gets scaled by the matrix
			// Get the scale from the matrix by transforming a unit vector
			CF_V2 scale_x = cf_sub(cf_mul(transform, cf_v2(1, 0)), cf_mul(transform, cf_v2(0, 0)));
			CF_V2 scale_y = cf_sub(cf_mul(transform, cf_v2(0, 1)), cf_mul(transform, cf_v2(0, 0)));
			float scale = cf_max(cf_len(scale_x), cf_len(scale_y));
			float transformed_radius = shape->capsule.r * scale;

			// Find the bounds that encompass both endpoints plus the radius
			float min_x = cf_min(a_transformed.x, b_transformed.x) - transformed_radius;
			float max_x = cf_max(a_transformed.x, b_transformed.x) + transformed_radius;
			float min_y = cf_min(a_transformed.y, b_transformed.y) - transformed_radius;
			float max_y = cf_max(a_transformed.y, b_transformed.y) + transformed_radius;

			return cf_make_aabb(cf_v2(min_x, min_y), cf_v2(max_x, max_y));
		} break;
	}
}

static void
collision_init(void* userdata, bent_world_t* world) {
	spatial_hash_init(userdata, bent_memctx(world), bgame_arena_pool, 128);
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
			bgame_transform_t* transform = &bent_get_comp_transform(world, ent)->current;
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			collider_t* collider = bent_get_comp_collider(world, ent);

			CF_Aabb transformed_aabb = make_aabb(collider->shape, mat);

			uint64_t id = (uint64_t)ent.index << 32 | (uint64_t)ent.gen;
			spatial_hash_insert(sh, transformed_aabb, id);
		}
	}

	if (update_mask == UPDATE_MASK_RENDER_POST) {
		cf_draw_push_layer(DRAW_LAYER_DEBUG);
		for (bent_index_t i = 0; i < num_entities; ++i) {
			bent_t ent = entities[i];
			bgame_transform_t* transform = &bent_get_comp_transform(world, ent)->current;
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			collider_t* collider = bent_get_comp_collider(world, ent);
			CF_Aabb transformed_aabb = make_aabb(collider->shape, mat);

			cf_draw_box(transformed_aabb, 0.5f, 0.5f);
			cf_draw_push();
			cf_draw_transform(mat);
			switch (collider->shape->type) {
				case CF_SHAPE_TYPE_NONE:
					break;
				case CF_SHAPE_TYPE_CIRCLE:
					cf_draw_circle(collider->shape->circle, 0.2f);
					break;
				case CF_SHAPE_TYPE_AABB:
					cf_draw_box(collider->shape->aabb, 0.2f, 0.2f);
					break;
				case CF_SHAPE_TYPE_CAPSULE:
					cf_draw_capsule(collider->shape->capsule, 0.2f);
					break;
				case CF_SHAPE_TYPE_POLY:
					cf_draw_polyline(
						collider->shape->poly.verts,
						collider->shape->poly.count,
						0.2f,
						true
					);
					break;
			}
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
	.require = BENT_COMP_LIST(&comp_transform, &comp_collider),
};
