#ifndef COLLISION_H
#define COLLISION_H

#include "../ecs.h"
#include "../spatial_hash.h"

spatial_hash_t*
get_collision_spatial_hash(float size);

bgame_collision_shape_t
bgame_transform_collision_shape(const bgame_collision_shape_t* shape, CF_M3x2 transform);

CF_Aabb
bgame_make_shape_aabb(const bgame_collision_shape_t* shape, CF_M3x2 transform);

#endif
