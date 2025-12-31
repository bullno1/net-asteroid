#ifndef COLLISION_H
#define COLLISION_H

#include "../ecs.h"
#include "../spatial_hash.h"

spatial_hash_t*
get_collision_spatial_hash(float size);

#endif
