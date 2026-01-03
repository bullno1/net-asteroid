#define BENT_DEFINE_COMPONENTS
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

static void
comp_sprite_init(void* data, void* arg) {
	sprite_t* sprite = data;
	sprite->asset = arg;
	if (arg == NULL) {
		sprite->instance = cf_sprite_defaults();
	} else {
		sprite->instance = *sprite->asset;
	}
}

BENT_DEFINE_COMP(comp_sprite) = {
	.size = sizeof(sprite_t),
	.init = comp_sprite_init,
};
