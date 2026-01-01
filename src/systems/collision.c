#include "collision.h"
#include "../spatial_hash.h"
#include <bgame/allocator.h>
#include <bgame/allocator/frame.h>
#include <bgame/collision.h>
#include <barray.h>
#include <cute.h>
#include <blog.h>
#include "debug_control.h"

BENT_DECLARE_SYS(sys_collision)

typedef struct {
	int event_id;
	spatial_hash_t sh;
	BHASH_TABLE(bent_t, CF_Aabb) aabb_cache;
	BHASH_TABLE(bent_index_t, collision_callback_fn_t) collision_callbacks;
	BHASH_SET(entity_pair_t) checked_pairs;
	barray(collision_event_t) collision_events;
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

	barray_free(sys->collision_events, bent_memctx(world));;
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

			// TODO: wrap around physic
			spatial_hash_insert(sh, transformed_aabb, ent);
		}

		// Check all cells with at least 2 objects
		bhash_clear(&sys->checked_pairs);
		barray_clear(sys->collision_events);
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

					// TODO: handle wrap around collision

					// Do they even care about each other?
					const collider_t* collider_a = bent_get_comp_collider(world, a);
					const collider_t* collider_b = bent_get_comp_collider(world, b);
					bool a_cares_about_b = (collider_a->mask & collider_b->group) > 0;
					bool b_cares_about_a = (collider_b->mask & collider_a->group) > 0;
					if (!(a_cares_about_b || b_cares_about_a)) { continue; }

					// First, check the AABBs against each other
					CF_Aabb aabb_of_a = sys->aabb_cache.values[bhash_find(&sys->aabb_cache, a)];
					CF_Aabb aabb_of_b = sys->aabb_cache.values[bhash_find(&sys->aabb_cache, b)];
					if (!cf_aabb_to_aabb(aabb_of_a, aabb_of_b)) { continue; }

					// Then, use the precise shapes
					const transform_t* comp_transform_a = bent_get_comp_transform(world, a);
					const transform_t* comp_transform_b = bent_get_comp_transform(world, b);
					CF_M3x2 transform_a = cf_make_transform_TSR(
						comp_transform_a->current.translation,
						comp_transform_a->current.scale,
						comp_transform_a->current.rotation
					);
					CF_M3x2 transform_b = cf_make_transform_TSR(
						comp_transform_b->current.translation,
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
							barray_push(sys->collision_events, event, bent_memctx(world));
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
			CF_M3x2 mat = cf_make_transform_TSR(transform->translation, transform->scale, transform->rotation);
			collider_t* collider = bent_get_comp_collider(world, ent);

			CF_Aabb transformed_aabb = { 0 };
			bhash_index_t index = bhash_find(&sys->aabb_cache, ent);
			if (bhash_is_valid(index)) {
				transformed_aabb = sys->aabb_cache.values[index];
			}

			cf_draw_box(transformed_aabb, 0.5f, 0.5f);
			cf_draw_push();
			cf_draw_transform(mat);
			switch (collider->shape->type) {
				case CF_SHAPE_TYPE_NONE:
					break;
				case CF_SHAPE_TYPE_CIRCLE:
					cf_draw_circle(collider->shape->data.circle, 0.2f);
					break;
				case CF_SHAPE_TYPE_AABB:
					cf_draw_box(collider->shape->data.aabb, 0.2f, 0.2f);
					break;
				case CF_SHAPE_TYPE_CAPSULE:
					cf_draw_capsule(collider->shape->data.capsule, 0.2f);
					break;
				case CF_SHAPE_TYPE_POLY: {
					const CF_Poly* poly = &collider->shape->data.poly;

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

		BARRAY_FOREACH_REF(record, sys->collision_events) {
			CF_Manifold manifold = record->manifold;

			cf_draw_push_color(cf_color_red());
			for (int i = 0; i < manifold.count; ++i) {
				cf_draw_circle_fill2(manifold.contact_points[i], 3.f);
				cf_draw_arrow(
					manifold.contact_points[i],
					cf_add(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
					0.1f,
					3.f
				);

				cf_draw_push_color(cf_color_green());
				cf_draw_arrow(
					manifold.contact_points[i],
					cf_sub(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
					0.1f,
					3.f
				);
				cf_draw_pop_color();
			}
			cf_draw_pop_color();
		}

		cf_draw_pop_layer();
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
