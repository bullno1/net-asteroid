#ifndef ECS_H
#define ECS_H

#include <bent.h>
#include <cute_sprite.h>
#include <bgame/transform.h>
#include <bgame/asset/collision_shape.h>

enum {
	UPDATE_MASK_FIXED_PRE    = 1 << 0,
	UPDATE_MASK_FIXED        = 1 << 1,
	UPDATE_MASK_FIXED_POST   = 1 << 2,
	UPDATE_MASK_VAR_PRE      = 1 << 3,
	UPDATE_MASK_VAR          = 1 << 4,
	UPDATE_MASK_VAR_POST     = 1 << 5,
	UPDATE_MASK_RENDER_PRE   = 1 << 6,
	UPDATE_MASK_RENDER       = 1 << 7,
	UPDATE_MASK_RENDER_POST  = 1 << 8,
	UPDATE_MASK_RENDER_DEBUG = 1 << 9,
};

enum {
	COLLISION_BIT_ASTEROID   = 1 << 0,
};

typedef enum {
	DRAW_LAYER_BACKGROUND = 0,
	DRAW_LAYER_COMMON,
	DRAW_LAYER_UI,
	DRAW_LAYER_DEBUG,

	DRAW_LAYER_COUNT,
} draw_layer_t;

typedef struct {
	bgame_transform_t current;
	bgame_transform_t previous;
} transform_t;

BENT_DECLARE_COMP(comp_transform)
BENT_DEFINE_COMP_ADDER_EX(comp_transform, transform_t, bgame_transform_t)
BENT_DEFINE_COMP_GETTER(comp_transform, transform_t)

BENT_DECLARE_COMP(comp_sprite)
BENT_DEFINE_COMP_ADDER(comp_sprite, CF_Sprite)
BENT_DEFINE_COMP_GETTER(comp_sprite, CF_Sprite)

typedef struct {
	draw_layer_t layer;
} renderable_t;

BENT_DECLARE_COMP(comp_renderable)
BENT_DEFINE_COMP_ADDER(comp_renderable, renderable_t)
BENT_DEFINE_COMP_GETTER(comp_renderable, renderable_t)

typedef struct {
	const bgame_collision_shape_t* shape;
	// When this entity's mask AND another's group > 1, a collision can be
	// triggered
	uint16_t mask;  // Mask is what we receive
	uint16_t group; // Group is what we send
} collider_t;

BENT_DECLARE_COMP(comp_collider)
BENT_DEFINE_COMP_ADDER(comp_collider, collider_t)
BENT_DEFINE_COMP_GETTER(comp_collider, collider_t)

typedef struct {
	CF_V2 velocity;
	float rotation;
} linear_motion_t;

BENT_DECLARE_COMP(comp_linear_motion)
BENT_DEFINE_COMP_GETTER(comp_linear_motion, linear_motion_t)
BENT_DEFINE_COMP_ADDER(comp_linear_motion, linear_motion_t)

BENT_DECLARE_COMP(comp_player_ship)
BENT_DEFINE_TAG_COMP_ADDER(comp_player_ship)

BENT_DECLARE_COMP(comp_ship_controller)
BENT_DEFINE_TAG_COMP_ADDER(comp_ship_controller)

static inline void
ecs_update_fixed(bent_world_t* world) {
	bent_run(world, UPDATE_MASK_FIXED_PRE);
	bent_run(world, UPDATE_MASK_FIXED);
	bent_run(world, UPDATE_MASK_FIXED_POST);
}

static inline void
ecs_update_variable(bent_world_t* world) {
	bent_run(world, UPDATE_MASK_VAR_PRE);
	bent_run(world, UPDATE_MASK_VAR);
	bent_run(world, UPDATE_MASK_VAR_POST);
}

static inline void
ecs_render(bent_world_t* world) {
	bent_run(world, UPDATE_MASK_RENDER_PRE);
	bent_run(world, UPDATE_MASK_RENDER);
	bent_run(world, UPDATE_MASK_RENDER_POST);
	bent_run(world, UPDATE_MASK_RENDER_DEBUG);
}

static inline void
snap_transform(transform_t* transform) {
	transform->previous = transform->current;
}

static inline CF_M3x2
get_interpolated_transform(transform_t* transform) {
	return bgame_lerp_transform(
		transform->previous,
		transform->current,
		CF_DELTA_TIME_INTERPOLANT
	);
}

#endif
