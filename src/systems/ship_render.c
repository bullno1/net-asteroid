#include "../ecs.h"
#include <cute.h>

typedef struct {
	CF_Sprite* fire;
} ship_render_t;

static void
ship_render_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	BENT_FOREACH_ENTITY(ent, entities) {
		int layer = bent_get_comp_renderable(world, ent)->layer;
		cf_draw_push_layer(layer);
		cf_draw_pop_layer();
	}
}

BENT_DEFINE_SYS(sys_ship_render) = {
	.update_mask = UPDATE_MASK_RENDER,
	.update = ship_render_update,
	.require = BENT_COMP_LIST(
		&comp_transform,
		&comp_renderable,
		&comp_sprite,
		&comp_ship
	),
};
