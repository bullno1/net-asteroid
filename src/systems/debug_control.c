#include "debug_control.h"
#include <bgame/scene.h>
#include <bgame/ui.h>
#include <bgame/allocator/tracked.h>
#include <barray.h>
#include <dcimgui.h>
#include <cute_input.h>

typedef struct {
	barray(bool) is_debug_enabled;
} debug_control_t;

static void
debug_control_cleanup(void* userdata, bent_world_t* world) {
	debug_control_t* sys = userdata;
	barray_free(sys->is_debug_enabled, bent_memctx(world));
}

static void
report_allocator_stats(
	const char* name,
	bgame_allocator_stats_t stats,
	void* userdata
) {
	ImGui_PushID(name);
	ImGui_SeparatorText(name);
	ImGui_Text("Current: %zu", stats.total);
	ImGui_Text("Peak: %zu", stats.peak);
	ImGui_PopID();
}

static void
debug_control_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	debug_control_t* sys = userdata;
	if (cf_key_just_pressed(CF_KEY_F12)) {
		ecs_set_debug_enabled(world, sys_debug_control, !ecs_is_debug_enabled(world, sys_debug_control));
		Clay_SetDebugModeEnabled(ecs_is_debug_enabled(world, sys_debug_control));
	}

	if (ecs_is_debug_enabled(world, sys_debug_control)) {
		if (ImGui_Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui_CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui_Button("Restart")) {
					bgame_reload_scene();
				}
			}

			if (ImGui_CollapsingHeader("Allocators", ImGuiTreeNodeFlags_DefaultOpen)) {

				bgame_enumerate_tracked_allocators(report_allocator_stats, NULL);
			}

			if (ImGui_CollapsingHeader("Other systems", ImGuiTreeNodeFlags_DefaultOpen)) {
				BENT_FOREACH_SYS(itr) {
					if (
						itr.sys.id != sys_debug_control.id  // Not this system
						&&
						itr.sys.def->update_mask & UPDATE_MASK_RENDER_DEBUG  // Support debug
					) {
						bent_index_t sys_index = itr.sys.id - 1;
						if (sys_index >= barray_len(sys->is_debug_enabled)) {
							barray_resize(sys->is_debug_enabled, sys_index + 1, bent_memctx(world));
						}
						ImGui_Checkbox(itr.name, &sys->is_debug_enabled[sys_index]);
					}
				}
			}
		}
		ImGui_End();
	}
}

BENT_DEFINE_SYS(sys_debug_control) = {
	.size = sizeof(debug_control_t),
	.update_mask = UPDATE_MASK_RENDER_DEBUG,
	.update = debug_control_update,
	.cleanup = debug_control_cleanup,
};

bool
ecs_is_debug_enabled(bent_world_t* world, bent_sys_reg_t sys) {
	if (sys.id == 0) { return false; }

	debug_control_t* debug_control = bent_get_sys_data(world, sys_debug_control);

	bent_index_t sys_index = sys.id - 1;
	if (sys_index >= barray_len(debug_control->is_debug_enabled)) {
		barray_resize(debug_control->is_debug_enabled, sys_index + 1, bent_memctx(world));
	}

	return debug_control->is_debug_enabled[sys_index];
}

void
ecs_set_debug_enabled(bent_world_t* world, bent_sys_reg_t sys, bool enabled) {
	if (sys.id == 0) { return; }

	debug_control_t* debug_control = bent_get_sys_data(world, sys_debug_control);

	bent_index_t sys_index = sys.id - 1;
	if (sys_index >= barray_len(debug_control->is_debug_enabled)) {
		barray_resize(debug_control->is_debug_enabled, sys_index + 1, bent_memctx(world));
	}

	debug_control->is_debug_enabled[sys_index] = enabled;
}
