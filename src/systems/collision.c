#include "collision.h"
#include "../spatial_hash.h"
#include <bgame/allocator.h>
#include <bgame/allocator/frame.h>
#include <bgame/collision.h>
#include <barray.h>
#include <cute.h>
#include <blog.h>
#include <dcimgui.h>
#include "debug_control.h"

BENT_DECLARE_SYS(sys_collision)

#define MIRROR_X_POSITIVE_BIT (bent_index_t)(1 << ((sizeof(bent_index_t) * CHAR_BIT) - 1))
#define MIRROR_X_NEGATIVE_BIT (bent_index_t)(1 << ((sizeof(bent_index_t) * CHAR_BIT) - 2))
#define MIRROR_Y_POSITIVE_BIT (bent_index_t)(1 << ((sizeof(bent_index_t) * CHAR_BIT) - 3))
#define MIRROR_Y_NEGATIVE_BIT (bent_index_t)(1 << ((sizeof(bent_index_t) * CHAR_BIT) - 4))

typedef struct {
	bool messaged_a;
	bool messaged_b;
	collision_event_t event;
} collision_record_t;

typedef struct {
	int event_id;
	spatial_hash_t sh;
	BHASH_TABLE(bent_t, CF_Aabb) aabb_cache;
	BHASH_TABLE(bent_index_t, collision_callback_fn_t) collision_callbacks;
	BHASH_SET(entity_pair_t) checked_pairs;
	barray(collision_record_t) collision_records;

	int num_aabb_checks;
	int num_detailed_checks;
} sys_collision_t;

static void
collision_init(void* userdata, bent_world_t* world) {
	sys_collision_t* sys = userdata;

	spatial_hash_init(&sys->sh, bent_memctx(world), bgame_arena_pool, 128);

	bhash_config_t bconfig = bhash_config_default();
	bconfig.memctx = bent_memctx(world);
	bconfig.removable = false;

	bhash_reinit(&sys->aabb_cache, bconfig);
	bhash_reinit_set(&sys->checked_pairs, bconfig);
	bhash_reinit(&sys->collision_callbacks, bconfig);
}

static void
collision_cleanup(void* userdata, bent_world_t* world) {
	sys_collision_t* sys = userdata;

	barray_free(sys->collision_records, bent_memctx(world));;
	bhash_cleanup(&sys->checked_pairs);
	bhash_cleanup(&sys->aabb_cache);
	bhash_cleanup(&sys->collision_callbacks);
	spatial_hash_cleanup(&sys->sh);
}

static void
collision_invoke_callbacks(
	sys_collision_t* sys,
	bent_world_t* world,
	bent_t entity,
	const collision_event_t* event
) {
	for (bhash_index_t i = 0; i < bhash_len(&sys->collision_callbacks); ++i) {
		bent_sys_reg_t listener = { .id = sys->collision_callbacks.keys[i] };
		collision_callback_fn_t callback = sys->collision_callbacks.values[i];

		if (bent_match(world, listener, entity)) {
			callback(bent_get_sys_data(world, listener), world, entity, event);
		}
	}
}

static void
collision_calculate_mirror(CF_Aabb aabb, CF_Aabb bound, int* mirror_x, int* mirror_y) {
	if (aabb.min.x < bound.min.x) { *mirror_x =  1; }
	if (aabb.max.x > bound.max.x) { *mirror_x = -1; }
	if (aabb.min.y < bound.min.y) { *mirror_y =  1; }
	if (aabb.max.y > bound.max.y) { *mirror_y = -1; }
}

static CF_Aabb
collision_mirror_aabb(CF_Aabb aabb, int mirror_x, int mirror_y, float width, float height) {
	CF_V2 offset = { width * mirror_x, height * mirror_y };
	return (CF_Aabb){
		.min = cf_add(aabb.min, offset),
		.max = cf_add(aabb.max, offset),
	};
}

static bent_t
collision_make_mirror_id(bent_t entity, int mirror_x, int mirror_y) {
	return (bent_t){
		.index = entity.index
			| (mirror_x > 0 ? MIRROR_X_POSITIVE_BIT : 0)
			| (mirror_x < 0 ? MIRROR_X_NEGATIVE_BIT : 0)
			| (mirror_y > 0 ? MIRROR_Y_POSITIVE_BIT : 0)
			| (mirror_y < 0 ? MIRROR_Y_NEGATIVE_BIT : 0),
		.gen = entity.gen,
	};
}

static bent_t
collision_decode_id(bent_t entity, int* mirror_x, int* mirror_y) {
	if (entity.index & MIRROR_X_POSITIVE_BIT) { *mirror_x =  1; }
	if (entity.index & MIRROR_X_NEGATIVE_BIT) { *mirror_x = -1; }
	if (entity.index & MIRROR_Y_POSITIVE_BIT) { *mirror_y =  1; }
	if (entity.index & MIRROR_Y_NEGATIVE_BIT) { *mirror_y = -1; }
	bent_index_t all_bits = MIRROR_X_POSITIVE_BIT | MIRROR_X_NEGATIVE_BIT | MIRROR_Y_POSITIVE_BIT | MIRROR_Y_NEGATIVE_BIT;
	return (bent_t){
		.index = entity.index & (~all_bits),
		.gen = entity.gen,
	};
}

static void
draw_debug_shape(
	CF_M3x2 transform,
	CF_Aabb aabb,
	const bgame_collision_shape_t* shape,
	int mirror_x,
	int mirror_y,
	float width,
	float height
) {
	CF_V2 offset = { width * mirror_x, height * mirror_y };

	cf_draw_push();
	cf_draw_translate_v2(offset);

	cf_draw_box(aabb, 0.5f, 0.5f);

	cf_draw_push();
	cf_draw_transform(transform);
	switch (shape->type) {
		case CF_SHAPE_TYPE_NONE:
			break;
		case CF_SHAPE_TYPE_CIRCLE:
			cf_draw_circle(shape->data.circle, 0.2f);
			break;
		case CF_SHAPE_TYPE_AABB:
			cf_draw_box(shape->data.aabb, 0.2f, 0.2f);
			break;
		case CF_SHAPE_TYPE_CAPSULE:
			cf_draw_capsule(shape->data.capsule, 0.2f);
			break;
		case CF_SHAPE_TYPE_POLY: {
			const CF_Poly* poly = &shape->data.poly;

			cf_draw_polyline(poly->verts, poly->count, 0.2f, true);

			for (int i = 0; i < poly->count; ++i) {
				CF_V2 point_a = poly->verts[i];
				CF_V2 point_b = poly->verts[(i + 1) % poly->count];
				CF_V2 midpoint = cf_mul(cf_add(point_a, point_b), 0.5f);
				cf_draw_arrow(
					midpoint,
					cf_add(midpoint, cf_mul(poly->norms[i], 10.f)),
					0.2f,
					1.5f
				);
			}
		} break;
	}
	cf_draw_pop();

	cf_draw_pop();
}

static void
collision_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	sys_collision_t* sys = userdata;
	spatial_hash_t* sh = &sys->sh;
	bool debug_enabled = ecs_is_debug_enabled(world, sys_collision);

	// Wrap around collision
	float width = cf_app_get_width();
	float height = cf_app_get_height();
	float half_width = width * 0.5f;
	float half_height = height * 0.5f;
	CF_Aabb screen_box = {
		.min = { -half_width, -half_height },
		.max = {  half_width,  half_height },
	};

	if (update_mask == UPDATE_MASK_FIXED_PRE) {
		// Put all colliders into a spatial hash
		spatial_hash_clear(sh);
		bhash_clear(&sys->aabb_cache);
		for (bent_index_t i = 0; i < num_entities; ++i) {
			bent_t ent = entities[i];
			bgame_transform_t* transform = &bent_get_comp_transform(world, ent)->current;
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			collider_t* collider = bent_get_comp_collider(world, ent);

			CF_Aabb transformed_aabb = bgame_make_aabb_from_shape(collider->shape, mat);
			bhash_put(&sys->aabb_cache, ent, transformed_aabb);  // Cache for checking later

			spatial_hash_insert(sh, transformed_aabb, ent);

			// Wrap around collision
			if (bent_has(world, ent, comp_wrap_around)) {
				int mirror_x = 0;
				int mirror_y = 0;

				collision_calculate_mirror(transformed_aabb, screen_box, &mirror_x, &mirror_y);

				if (mirror_x) {
					CF_Aabb mirrored = collision_mirror_aabb(
						transformed_aabb,
						mirror_x, 0,
						width, height
					);
					bent_t mirror_id = collision_make_mirror_id(ent, mirror_x, 0);
					spatial_hash_insert(sh, mirrored, mirror_id);
				}

				if (mirror_y) {
					CF_Aabb mirrored = collision_mirror_aabb(
						transformed_aabb,
						0, mirror_y,
						width, height
					);
					bent_t mirror_id = collision_make_mirror_id(ent, 0, mirror_y);
					spatial_hash_insert(sh, mirrored, mirror_id);
				}

				if (mirror_x && mirror_y) {
					CF_Aabb mirrored = collision_mirror_aabb(
						transformed_aabb,
						mirror_x, mirror_y,
						width, height
					);
					bent_t mirror_id = collision_make_mirror_id(ent, mirror_x, mirror_y);
					spatial_hash_insert(sh, mirrored, mirror_id);
				}
			}
		}

		// Check all cells with at least 2 objects
		bhash_clear(&sys->checked_pairs);
		barray_clear(sys->collision_records);
		sys->num_aabb_checks = 0;
		sys->num_detailed_checks = 0;
		for (bhash_index_t i = 0; i < bhash_len(&sh->cells); ++i) {
			spatial_hash_cell_entry_t* entry = sh->cells.values[i];
			if (entry->len == 1 && entry->next == NULL) { continue; }  // Only one object

			for (
				spatial_hash_cell_itr_t i = spatial_hash_make_itr(entry);
				!spatial_hash_itr_ended(i);
				i = spatial_hash_itr_next(i)
			) {
				for (
					spatial_hash_cell_itr_t j = spatial_hash_itr_next(i);
					!spatial_hash_itr_ended(j);
					j = spatial_hash_itr_next(j)
				) {
					bent_t a = spatial_hash_itr_data(i);
					bent_t b = spatial_hash_itr_data(j);

					int a_mirror_x = 0;
					int a_mirror_y = 0;
					int b_mirror_x = 0;
					int b_mirror_y = 0;
					a = collision_decode_id(a, &a_mirror_x, &a_mirror_y);
					b = collision_decode_id(b, &b_mirror_x, &b_mirror_y);

					// Check if we have checked this pair before
					entity_pair_t pair;
					// To ensure uniqueness of pairs, a must be "less than" b
					if (memcmp(&a, &b, sizeof(a)) < 0) {
						pair = (entity_pair_t){ a, b };
					} else {
						pair = (entity_pair_t){ b, a };
					}
					bhash_alloc_result_t alloc_result = bhash_alloc(&sys->checked_pairs, pair);
					if (!alloc_result.is_new) { continue; }
					sys->checked_pairs.keys[alloc_result.index] = pair;  // Mark this pair as checked

					// Do they even care about each other?
					const collider_t* collider_a = bent_get_comp_collider(world, a);
					const collider_t* collider_b = bent_get_comp_collider(world, b);
					bool a_cares_about_b = (collider_a->mask & collider_b->group) > 0;
					bool b_cares_about_a = (collider_b->mask & collider_a->group) > 0;
					if (!(a_cares_about_b || b_cares_about_a)) { continue; }


					// First, check the AABBs against each other
					++sys->num_aabb_checks;
					CF_Aabb aabb_of_a = sys->aabb_cache.values[bhash_find(&sys->aabb_cache, a)];
					CF_Aabb aabb_of_b = sys->aabb_cache.values[bhash_find(&sys->aabb_cache, b)];
					aabb_of_a = collision_mirror_aabb(aabb_of_a, a_mirror_x, a_mirror_y, width, height);
					aabb_of_b = collision_mirror_aabb(aabb_of_b, b_mirror_x, b_mirror_y, width, height);
					if (!cf_aabb_to_aabb(aabb_of_a, aabb_of_b)) { continue; }

					// Then, use the precise shapes
					++sys->num_detailed_checks;
					const transform_t* comp_transform_a = bent_get_comp_transform(world, a);
					const transform_t* comp_transform_b = bent_get_comp_transform(world, b);
					CF_V2 a_offset = { width * a_mirror_x, height * a_mirror_y };
					CF_V2 b_offset = { width * b_mirror_x, height * b_mirror_y };
					CF_M3x2 transform_a = cf_make_transform_TSR(
						cf_add(comp_transform_a->current.translation, a_offset),
						comp_transform_a->current.scale,
						comp_transform_a->current.rotation
					);
					CF_M3x2 transform_b = cf_make_transform_TSR(
						cf_add(comp_transform_b->current.translation, b_offset),
						comp_transform_b->current.scale,
						comp_transform_b->current.rotation
					);
					bgame_collision_shape_t shape_a = bgame_transform_collision_shape(
						collider_a->shape, transform_a
					);
					bgame_collision_shape_t shape_b = bgame_transform_collision_shape(
						collider_b->shape, transform_b
					);
					CF_Manifold manifold;
					cf_collide(
						&shape_a.data, NULL, shape_a.type,
						&shape_b.data, NULL, shape_b.type,
						&manifold
					);
					if (manifold.count > 0) {
						collision_event_t event = {
							.id = sys->event_id++,
							.manifold = manifold,
							.pair = pair,
						};

						if (a_cares_about_b) {
							collision_invoke_callbacks(sys, world, a, &event);
						}

						if (b_cares_about_a) {
							collision_invoke_callbacks(sys, world, b, &event);
						}

						if (debug_enabled) {
							collision_record_t record = {
								.messaged_a = a_cares_about_b,
								.messaged_b = b_cares_about_a,
								.event = event,
							};
							barray_push(sys->collision_records, record, bent_memctx(world));
						}
					}
				}
			}
		}
	}

	if (debug_enabled && update_mask == UPDATE_MASK_RENDER_DEBUG) {
		cf_draw_push_layer(DRAW_LAYER_DEBUG);
		for (bent_index_t i = 0; i < num_entities; ++i) {
			bent_t ent = entities[i];

			bgame_transform_t* transform = &bent_get_comp_transform(world, ent)->current;
			CF_M3x2 mat = cf_make_transform_TSR(
				transform->translation,
				transform->scale,
				transform->rotation
			);
			collider_t* collider = bent_get_comp_collider(world, ent);

			CF_Aabb transformed_aabb = { 0 };
			bhash_index_t index = bhash_find(&sys->aabb_cache, ent);
			if (bhash_is_valid(index)) {
				transformed_aabb = sys->aabb_cache.values[index];
			}

			draw_debug_shape(mat, transformed_aabb, collider->shape, 0, 0, width, height);

			if (bent_has(world, ent, comp_wrap_around)) {
				int mirror_x = 0;
				int mirror_y = 0;
				collision_calculate_mirror(transformed_aabb, screen_box, &mirror_x, &mirror_y);

				if (mirror_x) {
					draw_debug_shape(mat, transformed_aabb, collider->shape, mirror_x, 0, width, height);
				}

				if (mirror_y) {
					draw_debug_shape(mat, transformed_aabb, collider->shape, 0, mirror_y, width, height);
				}
				if (mirror_x & mirror_y) {
					draw_debug_shape(mat, transformed_aabb, collider->shape, mirror_x, mirror_y, width, height);
				}
			}
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

		BARRAY_FOREACH_REF(record, sys->collision_records) {
			CF_Manifold manifold = record->event.manifold;

			cf_draw_push_color(cf_color_red());
			for (int i = 0; i < manifold.count; ++i) {
				cf_draw_circle_fill2(manifold.contact_points[i], 3.f);

				if (record->messaged_b) {
					cf_draw_arrow(
						manifold.contact_points[i],
						cf_add(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
						0.1f,
						3.f
					);
				}

				if (record->messaged_a) {
					cf_draw_push_color(cf_color_green());
					cf_draw_arrow(
						manifold.contact_points[i],
						cf_sub(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
						0.1f,
						3.f
					);
					cf_draw_pop_color();
				}
			}
			cf_draw_pop_color();
		}

		cf_draw_pop_layer();

		if (ImGui_Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui_CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui_Text("Num aabb checks: %d", sys->num_aabb_checks);
				ImGui_Text("Num detailed checks: %d", sys->num_detailed_checks);
			}
		}
		ImGui_End();
	}
}

void
register_collision_callback(
	bent_world_t* world,
	bent_sys_reg_t listener,
	collision_callback_fn_t callback
) {
	if (listener.id == 0) { return; }

	sys_collision_t* sys = bent_get_sys_data(world, sys_collision);
	bhash_put(&sys->collision_callbacks, listener.id, callback);
}

BENT_DEFINE_SYS(sys_collision) = {
	.size = sizeof(sys_collision_t),
	.init = collision_init,
	.allow_reinit = true,
	.cleanup = collision_cleanup,
	.update_mask = UPDATE_MASK_FIXED_PRE | UPDATE_MASK_RENDER_DEBUG,
	.update = collision_update,
	.require = BENT_COMP_LIST(&comp_transform, &comp_collider),
};
