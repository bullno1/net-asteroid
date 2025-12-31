#include "ecs.h"

static void
comp_transform_init(void* data, void* arg) {
	transform_t* transform = data;
	bgame_transform_t* initial = arg;
	if (initial == NULL) {
		transform->previous = transform->current = bgame_transform_identity();
	} else {
		transform->previous = transform->current = *initial;
	}
}

BENT_DEFINE_COMP(comp_transform) = {
	.size = sizeof(transform_t),
	.init = comp_transform_init,
};

BENT_DEFINE_POD_COMP(comp_sprite, CF_Sprite)
BENT_DEFINE_POD_COMP(comp_collider, collider_t)
BENT_DEFINE_POD_COMP(comp_renderable, renderable_t)
