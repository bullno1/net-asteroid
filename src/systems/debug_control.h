#ifndef DEBUG_CONTROL_H
#define DEBUG_CONTROL_H

#include "../ecs.h"

BENT_DECLARE_SYS(sys_debug_control)

bool
ecs_is_debug_enabled(bent_world_t* world, bent_sys_reg_t sys);

void
ecs_set_debug_enabled(bent_world_t* world, bent_sys_reg_t sys, bool enabled);

#endif
