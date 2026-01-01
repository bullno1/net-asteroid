#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include <bhash.h>
#include <barena.h>
#include <bgame/allocator.h>
#include <bgame/utils.h>
#include <cute_math.h>

#ifndef SPATIAL_HASH_ID_TYPE
#include <bent.h>
#define SPATIAL_HASH_ID_TYPE bent_t
#endif

typedef struct spatial_hash_cell_entry_s spatial_hash_cell_entry_t;
typedef SPATIAL_HASH_ID_TYPE spatial_hash_id_t;

struct spatial_hash_cell_entry_s {
	spatial_hash_cell_entry_t* next;
	int len;

	spatial_hash_id_t data[];
};

typedef struct {
	const spatial_hash_cell_entry_t* current_entry;
	int current_index;
} spatial_hash_cell_itr_t;

typedef struct {
	int32_t x;
	int32_t y;
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
	barena_pool_t* arena_pool,
	float grid_size
);

void
spatial_hash_cleanup(spatial_hash_t* sh);

void
spatial_hash_clear(spatial_hash_t* sh);

void
spatial_hash_insert(spatial_hash_t* sh, CF_Aabb aabb, spatial_hash_id_t id);

spatial_hash_cell_entry_t*
spatial_hash_get_cell(spatial_hash_t* sh, spatial_hash_coord_t coord);

CF_Aabb
spatial_hash_get_cell_coord(spatial_hash_t* sh, spatial_hash_coord_t coord);

void
spatial_hash_get_range(
	spatial_hash_t* sh,
	CF_Aabb aabb,
	spatial_hash_coord_t* min_out,
	spatial_hash_coord_t* max_out
);

static inline spatial_hash_cell_itr_t
spatial_hash_make_itr(const spatial_hash_cell_entry_t* entry) {
	return (spatial_hash_cell_itr_t){
		.current_entry = entry,
		.current_index = 0,
	};
}

static inline bool
spatial_hash_itr_ended(const spatial_hash_cell_itr_t itr) {
	return itr.current_entry == NULL;  // There is never an empty entry
}

static inline spatial_hash_cell_itr_t
spatial_hash_itr_next(spatial_hash_cell_itr_t itr) {
	int next_index = itr.current_index + 1;
	if (next_index < itr.current_entry->len) {
		return (spatial_hash_cell_itr_t){
			.current_entry = itr.current_entry,
			.current_index = next_index,
		};
	} else {
		return (spatial_hash_cell_itr_t){
			.current_entry = itr.current_entry->next,
			.current_index = 0,
		};
	}
}

static inline spatial_hash_id_t
spatial_hash_itr_data(const spatial_hash_cell_itr_t itr) {
	return itr.current_entry->data[itr.current_index];
}

#endif
