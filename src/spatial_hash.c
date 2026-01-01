#include "spatial_hash.h"
#include <cute_multithreading.h>
#include <bminmax.h>
#include <bgame/reloadable.h>
#include <blog.h>

typedef void (*spatial_hash_callback_t)(
    int32_t cell_x,
    int32_t cell_y,
    void* ctx
);

static size_t spatial_hash_cached_entry_size = 0;
BGAME_PERSIST_VAR(spatial_hash_cached_entry_size)

static size_t
spatial_hash_entry_size(void) {
	if (spatial_hash_cached_entry_size == 0) {
		// Try to fit as many entries as possible into a cacheline
		// Cache the result since it can involve some syscalls
		spatial_hash_cached_entry_size = BMAX(
			cf_cacheline_size(),
			sizeof(spatial_hash_cell_entry_t) + sizeof(spatial_hash_id_t)
		);
		BLOG_DEBUG("Spatial hash entry size = %zu", spatial_hash_cached_entry_size);
	}
	return spatial_hash_cached_entry_size;
}

static spatial_hash_cell_entry_t*
spatial_hash_alloc_entry(barena_t* arena) {
	spatial_hash_cell_entry_t* entry = barena_memalign(
		arena,
		BMAX(cf_cacheline_size(), sizeof(spatial_hash_cell_entry_t) + sizeof(spatial_hash_id_t)),
		_Alignof(spatial_hash_cell_entry_t)
	);
	entry->len = 0;
	entry->next = NULL;
	return entry;
}

void
spatial_hash_init(
	spatial_hash_t* sh,
	bgame_allocator_t* allocator,
	barena_pool_t* arena_pool,
	float grid_size
) {

	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	hconfig.removable = false;
	bhash_reinit(&sh->cells, hconfig);

	bhash_clear(&sh->cells);
	if (sh->arena.pool != NULL) {
		barena_reset(&sh->arena);
	}
	barena_init(&sh->arena, arena_pool);

	sh->grid_size = grid_size;
}

void
spatial_hash_cleanup(spatial_hash_t* sh) {
	bhash_cleanup(&sh->cells);
	barena_reset(&sh->arena);
}

void
spatial_hash_clear(spatial_hash_t* sh) {
	bhash_clear(&sh->cells);
	barena_reset(&sh->arena);
}

void
spatial_hash_insert(spatial_hash_t* sh, CF_Aabb aabb, spatial_hash_id_t id) {
	spatial_hash_coord_t min, max;
	spatial_hash_get_range(sh, aabb, &min, &max);

	for (int32_t y = min.y; y <= max.y; ++y) {
		for (int32_t x = min.x; x <= max.x; ++x) {
			spatial_hash_coord_t coord = { x, y };

			bhash_alloc_result_t alloc_result = bhash_alloc(&sh->cells, coord);
			spatial_hash_cell_entry_t* entry;
			if (alloc_result.is_new) {
				entry = spatial_hash_alloc_entry(&sh->arena);
				sh->cells.keys[alloc_result.index] = coord;
				sh->cells.values[alloc_result.index] = entry;
			} else {
				entry = sh->cells.values[alloc_result.index];
			}

			// Check if it is full
			if (sizeof(spatial_hash_cell_entry_t) + entry->len * sizeof(spatial_hash_id_t) == spatial_hash_entry_size()) {
				spatial_hash_cell_entry_t* new_entry = spatial_hash_alloc_entry(&sh->arena);
				new_entry->next = entry;
				sh->cells.values[alloc_result.is_new] = new_entry;
				entry = new_entry;
			}

			entry->data[entry->len++] = id;
		}
	}
}

spatial_hash_cell_entry_t*
spatial_hash_get_cell(spatial_hash_t* sh, spatial_hash_coord_t coord) {
	bhash_index_t index =bhash_find(&sh->cells, coord);
	if (bhash_is_valid(index)) {
		return NULL;
	} else  {
		return NULL;
	}
}

void
spatial_hash_get_range(
	spatial_hash_t* sh,
	CF_Aabb aabb,
	spatial_hash_coord_t* min_out,
	spatial_hash_coord_t* max_out
) {
	float grid_size = sh->grid_size;
	int32_t min_x = (int32_t)floorf(aabb.min.x / grid_size);
	int32_t min_y = (int32_t)floorf(aabb.min.y / grid_size);
	int32_t max_x = (int32_t)floorf(aabb.max.x / grid_size);
	int32_t max_y = (int32_t)floorf(aabb.max.y / grid_size);

	*min_out = (spatial_hash_coord_t){ min_x, min_y };
	*max_out = (spatial_hash_coord_t){ max_x, max_y };
}

CF_Aabb
spatial_hash_get_cell_coord(spatial_hash_t* sh, spatial_hash_coord_t coord) {
	float grid_size = sh->grid_size;
	CF_Aabb aabb = {
		.min = {
			.x = coord.x * grid_size,
			.y = coord.y * grid_size,
		},
		.max = {
			.x = (coord.x + 1) * grid_size,
			.y = (coord.y + 1) * grid_size,
		},
	};
    return aabb;
}
