#ifndef COLLISION_H
#define COLLISION_H

#include "../ecs.h"

typedef struct {
	bent_t a;  // a must be "less than" b
	bent_t b;
} entity_pair_t;

typedef struct {
	// It is possible to be messaged twice back-to-back if a system matches both
	// objects in the pair
	// Using the id, it can dedupe
	int id;
	entity_pair_t pair;
	CF_Manifold manifold;
} collision_event_t;

typedef void (*collision_callback_fn_t)(
	void* userdata, bent_world_t* world, bent_t entity, const collision_event_t* event
);

void
register_collision_callback(
	bent_world_t* world,
	bent_sys_reg_t listener,
	collision_callback_fn_t callback
);

#endif
