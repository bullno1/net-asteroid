#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include <bhash.h>
#include <barena.h>
#include <bgame/allocator.h>
#include <cute_math.h>

typedef struct spatial_hash_cell_entry_s spatial_hash_cell_entry_t;

struct spatial_hash_cell_entry_s {
	spatial_hash_cell_entry_t* next;

	uint64_t data[];
};

typedef struct {
	int x;
	int y;
} spatial_hash_coord_t;

typedef struct {
	BHASH_TABLE(spatial_hash_coord_t, spatial_hash_cell_entry_t*) cells;
	barena_t arena;
	float grid_size;
} spatial_hash_t;

void
spatial_hash_init(
	spatial_hash_t* sh,
	bgame_allocator_t* allocator,
	barena_pool_t* arena_pool
);

void
spatial_hash_cleanup(spatial_hash_t* sh);

void
spatial_hash_clear(spatial_hash_t* sh);

void
spatial_hash_insert(spatial_hash_t* sh, CF_Aabb aabb, uint64_t id);

spatial_hash_cell_entry_t*
spatial_hash_get_cell(spatial_hash_t* sh, spatial_hash_coord_t coord);

void
spatial_hash_get_range(
	spatial_hash_t* sh,
	CF_Aabb aabb,
	spatial_hash_coord_t* min_ptr,
	spatial_hash_coord_t* max_ptr
);

#endif
