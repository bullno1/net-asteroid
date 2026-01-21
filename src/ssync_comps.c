#include "ecs.h"
#include "slopsync.h"
#include <blog.h>

SSYNC_TYPED_COMP(comp_transform, transform_t) {
	ssync_prop_float(ctx, &comp->current.translation.x, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_POSITION_X);
	ssync_prop_float(ctx, &comp->current.translation.y, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_POSITION_Y);
	ssync_prop_float(ctx, &comp->current.rotation, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_ROTATION);
}

SSYNC_TYPED_COMP(comp_renderable, renderable_t) {
	ssync_prop_any_int(ctx, &comp->layer, SSYNC_PROP_DEFAULT);
}

SSYNC_TYPED_COMP(comp_sprite, sprite_t) {
	CF_Sprite* asset;

	if (ssync_mode(ctx) == SSYNC_MODE_WRITE) {
		asset = comp->asset;
	}

	ssync_prop_asset(ctx, &comp->asset);

	if (ssync_mode(ctx) == SSYNC_MODE_READ && asset != comp->asset) {
		comp->instance = *asset;
	}
}

SSYNC_TYPED_COMP(comp_collider, collider_t) {
	ssync_prop_asset(ctx, &comp->shape);
	ssync_prop_any_int(ctx, &comp->mask, SSYNC_PROP_DEFAULT);
	ssync_prop_any_int(ctx, &comp->group, SSYNC_PROP_DEFAULT);
}

SSYNC_TYPED_COMP(comp_linear_motion, linear_motion_t) {
	ssync_prop_float(ctx, &comp->velocity.x, 3, SSYNC_PROP_DEFAULT);
	ssync_prop_float(ctx, &comp->velocity.y, 3, SSYNC_PROP_DEFAULT);
	ssync_prop_float(ctx, &comp->rotation, 3, SSYNC_PROP_ROTATION);
}

SSYNC_TYPED_COMP(comp_projectile, projectile_t) {
	ssync_prop_any_int(ctx, &comp->type, SSYNC_PROP_DEFAULT);
}

SSYNC_TYPED_COMP(comp_ship, ship_t) {
	uint8_t states;
	if (ssync_mode(ctx) == SSYNC_MODE_WRITE) {
		states =
			  (int)comp->thrusting << 0
			| (int)comp->braking << 1
			| (int)comp->firing << 2;
	}
	ssync_prop_any_int(ctx, &states, SSYNC_PROP_DEFAULT);
	if (ssync_mode(ctx) == SSYNC_MODE_WRITE) {
		comp->thrusting = states & (1 << 0);
		comp->braking = states & (1 << 1);
		comp->firing = states & (1 << 2);
	}

	ssync_prop_float(ctx, &comp->turning, 3, SSYNC_PROP_DEFAULT);
	ssync_prop_float(ctx, &comp->charge_progress, 3, SSYNC_PROP_DEFAULT);
}

SSYNC_TAG_COMP(comp_asteroid)
SSYNC_TAG_COMP(comp_wrap_around)
SSYNC_TAG_COMP(comp_offscreen_cull)
