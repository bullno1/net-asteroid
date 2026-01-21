// vim: set foldmethod=marker foldlevel=0:
#include "slopsync.h"
#include <slopnet.h>
#include <bent.h>
#include <bgame/allocator/frame.h>
#include <blog.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <bhash.h>
#include <barray.h>
#include "ecs.h"

BENT_DEFINE_POD_COMP(comp_slopsync_link, slopsync_link_t)

BENT_DECLARE_SYS(sys_slopsync)

AUTOLIST_IMPL(ssync__comps)

typedef struct {
	ssync_t* ssync;
	bent_world_t* world;

	barray(slopsync_comp_spec_t) comp_specs;
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

	ssync_config_t ssync_config = {
		.max_message_size = snet_max_message_size(),
		.realloc = sys_ssync_realloc,
		.create_obj = sys_ssync_create,
		.destroy_obj = sys_ssync_destroy,
		.add_prop_group = sys_ssync_add_prop_group,
		.rem_prop_group = sys_ssync_rem_prop_group,
		.has_prop_group = sys_ssync_has_prop_group,
		.sync = sys_ssync_sync,

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

	if (update_mask == UPDATE_MASK_VAR_PRE) {
		ssync_update(sys->ssync, CF_DELTA_TIME);
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
ssync_bent_sync_static_schema(bent_world_t* world, const ssync_static_schema_t* schema) {
	sys_slopsync_t* sys = bent_get_sys_data(world, sys_slopsync);
	size_t size = ssync_info(sys->ssync).schema_size;
	char* content = bgame_alloc_for_frame(size, _Alignof(uint8_t));
	ssync_write_schema(sys->ssync, content);
	uint64_t hash = bhash_hash(content, size);

	if (size == schema->size && hash == schema->hash) {
		return;
	}

	BLOG_INFO("Updating slopsync schema at %s", schema->filename);

	FILE* file = fopen(schema->filename, "wb");
	if (file == NULL) {
		BLOG_ERROR("Could not open file for writing: %s", strerror(errno));
		return;
	}

	fprintf(
		file,
		"#pragma once\n\n"
		"#include \"slopsync.h\"\n\n"
	);

	fprintf(file, "static const ssync_static_schema_t ssync_schema = {\n");
	fprintf(file, "\t.filename = __FILE__,\n");
	fprintf(file, "\t.size = %zuull,\n", size);
	fprintf(file, "\t.hash = %" PRIu64 "ull,\n", hash);
	fprintf(file, "\t.content =\n");

	int line_width = 76;
	for (size_t i = 0; i < size; i += line_width) {
		int remaining_len = (int)size - i;
		int len = line_width < remaining_len ? line_width : remaining_len;
		fprintf(file, "\t\t\"%.*s\"\n", len, &content[i]);
	}

	fprintf(file, "};\n");

	fclose(file);
}

void
(ssync_prop_asset)(ssync_ctx_t* ctx, const void** asset) {
}
