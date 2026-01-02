#include "../ecs.h"
#include <bgame/allocator.h>
#include <bgame/asset/collision_shape.h>
#include <bhash.h>

// Automatically make an AABB collider shape for entities without one

typedef union {
	uint64_t easy_sprite_id;
	const char* name;
} sprite_id_t;

typedef struct {
	BHASH_TABLE(sprite_id_t, bgame_collision_shape_t*) shapes;
} collider_maker_t;

static void
collider_maker_init(void* userdata, bent_world_t* world) {
	collider_maker_t* sys = userdata;
	bhash_config_t config = bhash_config_default();
	config.memctx = bent_memctx(world);
	config.removable = false;
	bhash_init(&sys->shapes, config);
}

static void
collider_maker_cleanup(void* userdata, bent_world_t* world) {
	collider_maker_t* sys = userdata;
	for (bhash_index_t i = 0; i < bhash_len(&sys->shapes); ++i) {
		bgame_free(sys->shapes.values[i], bent_memctx(world));;
	}
	bhash_cleanup(&sys->shapes);
}

static void
collider_maker_add(
	void* userdata, bent_world_t* world, bent_t entity
) {
	collider_maker_t* sys = userdata;

	collider_t* collider = bent_get_comp_collider(world, entity);
	if (collider->shape == NULL) {
		const CF_Sprite* sprite = &bent_get_comp_sprite(world, entity)->instance;
		// The identity of a sprite is either easy sprite or name
		sprite_id_t sprite_id = { 0 };
		if (sprite->easy_sprite_id > 0) {
			sprite_id.easy_sprite_id = sprite->easy_sprite_id;
		} else {
			sprite_id.name = sprite->name;
		}
		bhash_alloc_result_t alloc_result = bhash_alloc(&sys->shapes, sprite_id);

		if (alloc_result.is_new) {  // New shape
			bgame_collision_shape_t* shape = bgame_malloc(
				sizeof(bgame_collision_shape_t),
				bent_memctx(world)
			);
			shape->type = CF_SHAPE_TYPE_AABB;
			shape->data.aabb = cf_make_aabb_center_half_extents(
				cf_v2(0.f, 0.f),
				cf_v2(sprite->w * 0.5f, sprite->h * 0.5f)
			);

			sys->shapes.keys[alloc_result.index] = sprite_id;
			sys->shapes.values[alloc_result.index] = shape;
		}

		collider->shape = sys->shapes.values[alloc_result.index];
	}
}

BENT_DEFINE_SYS(sys_collider_maker) = {
	.size = sizeof(collider_maker_t),
	.init = collider_maker_init,
	.cleanup = collider_maker_cleanup,
	.require = BENT_COMP_LIST(&comp_sprite, &comp_collider),
	.add = collider_maker_add,
};
