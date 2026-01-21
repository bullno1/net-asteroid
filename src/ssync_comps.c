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
