#include "../ecs.h"
#include <cute.h>
#include <bgame/asset/sprite.h>
#include "../assets.h"

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
		CF_M3x2 transform = get_interpolated_transform(bent_get_comp_transform(world, ent));
		sprite_t* sprite = bent_get_comp_sprite(world, ent);
		ship_t* ship = bent_get_comp_ship(world, ent);

		cf_draw_push_layer(layer);
		cf_draw_push();

		cf_draw_transform(transform);
		cf_draw_translate_v2(sprite->asset->pivots[sprite->instance.frame_index]);

		CF_V2 left_exhaust = cf_center(cf_sprite_get_slice(sprite->asset, "left-exhaust"));
		CF_V2 right_exhaust = cf_center(cf_sprite_get_slice(sprite->asset, "right-exhaust"));
		CF_V2 left_brake = cf_center(cf_sprite_get_slice(sprite->asset, "left-brake"));
		CF_V2 right_brake = cf_center(cf_sprite_get_slice(sprite->asset, "right-brake"));

		if (ship->thrusting) {
			CF_Sprite exhaust = *spr_exhaust;
			cf_sprite_play(&exhaust, "thrust");
			exhaust.transform.p = left_exhaust;
			cf_draw_sprite(&exhaust);
			exhaust.transform.p = right_exhaust;
			cf_draw_sprite(&exhaust);
		}

		if (ship->turning > 0.f) {
			CF_Sprite exhaust = *spr_exhaust;
			cf_sprite_play(&exhaust, "turn");
			exhaust.transform.p = left_brake;
			cf_draw_sprite(&exhaust);
		} else if (ship->turning < 0.f) {
			CF_Sprite exhaust = *spr_exhaust;
			cf_sprite_play(&exhaust, "turn");
			exhaust.transform.p = right_brake;
			cf_draw_sprite(&exhaust);
		}

		if (ship->braking) {
			CF_Sprite exhaust = *spr_exhaust;
			cf_sprite_play(&exhaust, "turn");

			cf_draw_push();
			cf_draw_translate_v2(left_brake);
			cf_draw_rotate(0.5f * CF_PI);
			cf_draw_sprite(&exhaust);
			cf_draw_pop();

			cf_draw_push();
			cf_draw_translate_v2(right_brake);
			cf_draw_rotate(-0.5f * CF_PI);
			cf_draw_sprite(&exhaust);
			cf_draw_pop();
		}

		cf_draw_pop();
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
