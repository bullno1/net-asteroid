// vim: set foldmethod=marker foldlevel=0:
#include "slopsync.h"
#include <slopnet.h>
#include <cute.h>
#include <bent.h>
#include <bgame/allocator/frame.h>
#include <bgame/asset.h>
#include <blog.h>
#include <string.h>
#include <inttypes.h>
#include <bhash.h>
#include <barray.h>
#include <dcimgui.h>
#include "ecs.h"
#include "globals.h"
#include "systems/debug_control.h"

BENT_DEFINE_POD_COMP(comp_slopsync_link, slopsync_link_t)
BENT_DEFINE_TAG_COMP(comp_slopsync_remote)

BENT_DECLARE_SYS(sys_slopsync)

AUTOLIST_IMPL(ssync__comps)

typedef struct {
	ssync_t* ssync;
	bent_world_t* world;
	snet_t* snet;

	barray(slopsync_comp_spec_t) comp_specs;

	BHASH_TABLE(const void*, uint32_t) asset_to_hash;
	BHASH_TABLE(uint32_t, const void*) hash_to_asset;

	BHASH_TABLE(ssync_net_id_t, bent_t) net_to_local;
} sys_slopsync_t;

static bent_t
ssync_net_to_local(sys_slopsync_t* sys, ssync_net_id_t net_id) {
	bhash_index_t index = bhash_find(&sys->net_to_local, net_id);
	if (bhash_is_valid(index)) {
		return sys->net_to_local.values[index];
	} else {
		return (bent_t){ 0 };
	}
}

static void*
sys_ssync_realloc(
	void* userdata,
	void* ptr,
	size_t size
) {
	sys_slopsync_t* sys = userdata;
	return bgame_realloc(ptr, size, bent_memctx(sys->world));
}

// ssync client {{{

static void
sys_ssync_create(void* userdata, ssync_net_id_t net_id) {
	sys_slopsync_t* sys = userdata;

	bent_t local = bent_create(sys->world);
	bent_add_comp_slopsync_remote(sys->world, local);
	bent_add_comp_slopsync_link(sys->world, local, &(slopsync_link_t){
		.id = net_id,
		.flags = ssync_obj_info(sys->ssync, net_id)->flags,
	});
}

static void
sys_ssync_destroy(void* userdata, ssync_net_id_t net_id) {
	sys_slopsync_t* sys = userdata;
	bent_destroy(sys->world, ssync_net_to_local(sys, net_id));
}

static void
sys_ssync_add_prop_group(
	void* userdata,
	ssync_net_id_t net_id, ssync_local_id_t prop_group_id
) {
	sys_slopsync_t* sys = userdata;
	bent_t local = ssync_net_to_local(sys, net_id);
	bent_add(sys->world, local, (bent_comp_reg_t){ .id = prop_group_id }, NULL);
}

static void
sys_ssync_rem_prop_group(
	void* userdata,
	ssync_net_id_t net_id, ssync_local_id_t prop_group_id
) {
	sys_slopsync_t* sys = userdata;
	bent_t local = ssync_net_to_local(sys, net_id);
	bent_remove(sys->world, local, (bent_comp_reg_t){ .id = prop_group_id });
}

static bool
sys_ssync_has_prop_group(
	void* userdata,
	ssync_net_id_t net_id,
	ssync_local_id_t prop_group_id
) {
	sys_slopsync_t* sys = userdata;
	bent_t local = ssync_net_to_local(sys, net_id);
	return bent_has(sys->world, local, (bent_comp_reg_t){ .id = prop_group_id });
}

static void
sys_ssync_sync(
	void* userdata,
	ssync_ctx_t* ctx,
	ssync_net_id_t obj_id
) {
	sys_slopsync_t* sys = userdata;
	bent_t entity = ssync_net_to_local(sys, obj_id);

	BARRAY_FOREACH_REF(spec, sys->comp_specs) {
		if (ssync_prop_group(ctx, spec->comp->id)) {
			spec->sync_fn(ctx, sys->world, entity);
		}
	}
}

static void
sys_ssync_send_msg(void* userdata, ssync_blob_t message, bool reliable) {
	sys_slopsync_t* sys = userdata;
	if (sys->snet != NULL) {
		snet_send(sys->snet, (snet_blob_t){ .ptr = message.data, .size = message.size }, reliable);
	}
}

// }}}

// bent system {{{

static int
slopsync_comp_spec_cmp(const void* lhs_ptr, const void* rhs_ptr) {
	const slopsync_comp_spec_t* lhs = lhs_ptr;
	const slopsync_comp_spec_t* rhs = rhs_ptr;

	return strcmp(lhs->name, rhs->name);
}

static void
sys_ssync_init(void* userdata, bent_world_t* world) {
	sys_slopsync_t* sys = userdata;
	sys->world = world;

	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = bent_memctx(world);
	bhash_reinit(&sys->net_to_local, hconfig);

	// Gather component sync functions
	barray_clear(sys->comp_specs);
	AUTOLIST_FOREACH(itr, ssync__comps) {
		slopsync_comp_spec_t* spec = itr->value_addr;
		barray_push(sys->comp_specs, *spec, bent_memctx(world));
	}

	// Sort for a deterministic list
	qsort(
		sys->comp_specs,
		barray_len(sys->comp_specs),
		sizeof(sys->comp_specs[0]),
		slopsync_comp_spec_cmp
	);
	BARRAY_FOREACH_REF(spec, sys->comp_specs) {
		BLOG_DEBUG("Registered %s", spec->name);
	}

	bhash_reinit(&sys->hash_to_asset, hconfig);
	bhash_reinit(&sys->asset_to_hash, hconfig);
	bhash_clear(&sys->hash_to_asset);
	bhash_clear(&sys->asset_to_hash);
	BGAME_FOREACH_DEFINED_ASSET(asset) {
		uint32_t hash = (uint32_t)bhash_hash(asset->name, strlen(asset->name));
		const void** asset_var = asset->var;
		const void* asset_value = *asset_var;

		bhash_put(&sys->hash_to_asset, hash, asset_value);
		bhash_put(&sys->asset_to_hash, asset_value, hash);
		BLOG_DEBUG("%" PRIx32 " -> %s (%p)", hash, asset->name, (void*)asset_value);
	}

	ssync_config_t ssync_config = {
		.max_message_size = snet_max_message_size(),
		.realloc = sys_ssync_realloc,
		.create_obj = sys_ssync_create,
		.destroy_obj = sys_ssync_destroy,
		.add_prop_group = sys_ssync_add_prop_group,
		.rem_prop_group = sys_ssync_rem_prop_group,
		.has_prop_group = sys_ssync_has_prop_group,
		.sync = sys_ssync_sync,
		.send_msg = sys_ssync_send_msg,
		.interpolation_ratio = 2.0f,

		.userdata = sys,
	};
	ssync_reinit(&sys->ssync, &ssync_config);
}

static void
sys_ssync_cleanup(void* userdata, bent_world_t* world) {
	sys_slopsync_t* sys = userdata;

	ssync_cleanup(sys->ssync);
	barray_free(sys->comp_specs, bent_memctx(world));
	bhash_cleanup(&sys->net_to_local);
	bhash_cleanup(&sys->asset_to_hash);
	bhash_cleanup(&sys->hash_to_asset);
}

static void
sys_ssync_add(void* userdata, bent_world_t* world, bent_t entity) {
	sys_slopsync_t* sys = userdata;
	slopsync_link_t* link = bent_get_comp_slopsync_link(world, entity);

	if (link->id.bin == 0) {
		link->id = ssync_create(sys->ssync, link->flags);
	}

	bhash_put(&sys->net_to_local, link->id, entity);
}

static void
sys_ssync_remove(void* userdata, bent_world_t* world, bent_t entity) {
	sys_slopsync_t* sys = userdata;
	slopsync_link_t* link = bent_get_comp_slopsync_link(world, entity);

	const ssync_obj_info_t* obj_info = ssync_obj_info(sys->ssync, link->id);
	if (obj_info && obj_info->is_local) {
		ssync_destroy(sys->ssync, link->id);
	}

	bhash_remove(&sys->net_to_local, link->id);
}

static void
sys_ssync_update(
	void* userdata,
	bent_world_t* world,
	bent_mask_t update_mask,
	bent_t* entities,
	bent_index_t num_entities
) {
	sys_slopsync_t* sys = userdata;
	bool debug_enabled = ecs_is_debug_enabled(world, sys_slopsync);

	if (update_mask == UPDATE_MASK_VAR_PRE) {
		if (sys->snet != NULL && snet_lobby_state(sys->snet) == SNET_JOINED_GAME) {
			const snet_event_t* event;
			while ((event = snet_next_event(sys->snet)) != NULL) {
				if (event->type == SNET_EVENT_MESSAGE) {
					ssync_process_message(sys->ssync, (ssync_blob_t){
						.data = event->message.data.ptr,
						.size = event->message.data.size,
					});
				}
			}

			ssync_update(sys->ssync, CF_DELTA_TIME);
		}
	}

	if (debug_enabled && update_mask == UPDATE_MASK_RENDER_DEBUG) {
		if (ImGui_Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui_CollapsingHeader("Network", ImGuiTreeNodeFlags_DefaultOpen)) {
				ssync_info_t info = ssync_info(sys->ssync);

				ImGui_Text("Incoming snapshots: %d", info.num_incoming_snapshots);
				ImGui_Text("Outgoing snapshots: %d", info.num_outgoing_snapshots);

				snet_lobby_state_t lobby_state = snet_lobby_state(g_snet);
				const char* lobby_state_names[] = {
					"IN_LOBBY",
					"LISTING_GAMES",
					"CREATING_GAME",
					"JOINING_GAME",
					"JOINED_GAME",
				};
				ImGui_LabelText("Snet state", "%s", lobby_state_names[lobby_state]);
			}
		}
		ImGui_End();

		cf_draw_push_layer(DRAW_LAYER_DEBUG);
		for (bhash_index_t i = 0; i < bhash_len(&sys->net_to_local); ++i) {
			ssync_net_id_t id = sys->net_to_local.keys[i];
			const ssync_obj_info_t* obj_info = ssync_obj_info(sys->ssync, id);

			_Alignas(ssync_obj_debug_info_t) char debug_info_buf[sizeof(ssync_obj_debug_info_t) + sizeof(ssync_obj_spatial_info_t) * 10];
			ssync_obj_debug_info_t* debug_info = (void*)debug_info_buf;
			debug_info->num_snapshots = 10;
			ssync_obj_debug_info(sys->ssync, id, debug_info);

			CF_Color color = obj_info->is_local ? cf_color_green() : cf_color_red();
			if (debug_info->next_snapshot >= 0 && debug_info->prev_snapshot >= 0) {
				cf_draw_push_color(color);
				ssync_obj_spatial_info_t from = debug_info->snapshots[debug_info->prev_snapshot];
				ssync_obj_spatial_info_t to   = debug_info->snapshots[debug_info->next_snapshot];
				CF_V2 from_point = cf_v2(from.x, from.y);
				CF_V2 to_point = cf_v2(to.x, to.y);
				cf_draw_arrow(from_point, to_point, 0.4f, 0.7f);

				CF_V2 interp_point = cf_lerp(from_point, to_point, debug_info->interpolant);
				cf_draw_circle_fill2(interp_point, 1.5f);

				cf_draw_pop_color();
			}

			for (int i = 0; i < debug_info->num_snapshots; ++i) {
				color.a = cf_lerp(1.0, 0.2, (float)i / (float)debug_info->num_snapshots);
				cf_draw_push_color(color);
				ssync_obj_spatial_info_t spatial_info = debug_info->snapshots[i];
				float radius = cf_lerp(cf_max(5.f, spatial_info.radius), 0.1, (float)i / (float)debug_info->num_snapshots);
				cf_draw_circle2(
					cf_v2(spatial_info.x, spatial_info.y),
					radius,
					0.5f
				);
				cf_draw_pop_color();
			}
		}
		cf_draw_pop_layer();
	}
}

// }}}

BENT_DEFINE_SYS(sys_slopsync) = {
	.size = sizeof(sys_slopsync_t),
	.init = sys_ssync_init,
	.cleanup = sys_ssync_cleanup,
	.allow_reinit = true,
	.require = BENT_COMP_LIST(&comp_slopsync_link),
	.add = sys_ssync_add,
	.remove = sys_ssync_remove,
	.update_mask = UPDATE_MASK_VAR_PRE | UPDATE_MASK_RENDER_DEBUG,
	.update = sys_ssync_update,
};

void
(ssync_prop_asset)(ssync_ctx_t* ctx, const void** asset_ptr) {
	sys_slopsync_t* sys = ssync_ctx_userdata(ctx);

	uint32_t hash;
	if (ssync_mode(ctx) == SSYNC_MODE_WRITE) {
		const void* asset = *asset_ptr;
		bhash_index_t index = bhash_find(&sys->asset_to_hash, asset);
		if (bhash_is_valid(index)) {
			hash = sys->asset_to_hash.values[index];
		} else {
			hash = 0;
		}
	}

	ssync_prop_u32(ctx, &hash, SSYNC_PROP_DEFAULT);

	if (ssync_mode(ctx) == SSYNC_MODE_READ) {
		bhash_index_t index = bhash_find(&sys->hash_to_asset, hash);
		if (bhash_is_valid(index)) {
			*asset_ptr = sys->hash_to_asset.values[index];
		} else {
			*asset_ptr = NULL;
		}
	}
}

void
ssync_attach_snet(bent_world_t* world, struct snet_s* snet) {
	sys_slopsync_t* sys = bent_get_sys_data(world, sys_slopsync);
	sys->snet = snet;
}

ssync_t*
ssync_client(bent_world_t* world) {
	sys_slopsync_t* sys = bent_get_sys_data(world, sys_slopsync);
	return sys->ssync;
}
