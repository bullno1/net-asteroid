#ifndef ASTEROID_H
#define ASTEROID_H

#include <cute_math.h>
#include "ecs.h"

typedef struct {
	CF_V2 velocity;
	float rotation;
} asteroid_t;

BENT_DECLARE_COMP(comp_asteroid)
BENT_DEFINE_COMP_GETTER(comp_asteroid, asteroid_t)
BENT_DEFINE_COMP_ADDER(comp_asteroid, asteroid_t)

#endif
