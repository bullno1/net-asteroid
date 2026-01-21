#ifndef SLOPSYNC_BENT_INTEGRATION_H
#define SLOPSYNC_BENT_INTEGRATION_H

#include <slopsync/client.h>
#include <autolist.h>
#include <bent.h>

struct snet_s;

#define ssync_prop_asset(CTX, PTR) ssync_prop_asset((CTX), (const void**)(PTR))

#define SSYNC_COMP(NAME) \
	void ssync_fn__##NAME(ssync_ctx_t* ctx, bent_world_t* world, bent_t entity); \
	slopsync_comp_spec_t ssync_spec__##NAME = { \
		.name = #NAME, \
		.comp = &NAME, \
		.sync_fn = ssync_fn__##NAME, \
	}; \
	AUTOLIST_ADD_ENTRY(ssync__comps, NAME, ssync_spec__##NAME) \
	void ssync_fn__##NAME(ssync_ctx_t* ctx, bent_world_t* world, bent_t entity)

#define SSYNC_TAG_COMP(NAME) SSYNC_COMP(NAME) {}

#define SSYNC_TYPED_COMP(NAME, TYPE) \
	static void ssync_fn_sync__##NAME(ssync_ctx_t* ctx, bent_world_t* world, bent_t entity, TYPE* comp); \
	static void ssync_fn__##NAME(ssync_ctx_t* ctx, bent_world_t* world, bent_t entity) { \
		TYPE* comp = bent_get_##NAME(world, entity); \
		ssync_fn_sync__##NAME(ctx, world, entity, comp); \
	}\
	static slopsync_comp_spec_t ssync_spec__##NAME = { \
		.name = #NAME, \
		.comp = &NAME, \
		.sync_fn = ssync_fn__##NAME, \
	}; \
	AUTOLIST_ADD_ENTRY(ssync__comps, NAME, ssync_spec__##NAME) \
	static void ssync_fn_sync__##NAME(ssync_ctx_t* ctx, bent_world_t* world, bent_t entity, TYPE* comp)

typedef struct {
	const char* filename;
	uint64_t hash;
	size_t size;
	const char* content;
} ssync_static_schema_t;

typedef struct {
	ssync_net_id_t id;
	ssync_obj_flags_t flags;
} slopsync_link_t;

typedef void (*slopsync_fn_t)(
	ssync_ctx_t* ctx,
	bent_world_t* world,
	bent_t entity
);

typedef struct {
	const char* name;
	const bent_comp_reg_t* comp;
	slopsync_fn_t sync_fn;
} slopsync_comp_spec_t;

BENT_DECLARE_COMP(comp_slopsync_link)
BENT_DEFINE_COMP_ADDER(comp_slopsync_link, slopsync_link_t)
BENT_DEFINE_COMP_GETTER(comp_slopsync_link, slopsync_link_t)

typedef void (*ssync_control_callback_fn_t)(
	void* userdata,
	bent_world_t* world,
	ssync_player_id_t sender,
	bent_t entity,
	ssync_blob_t command
);

void
ssync_register_control_callback(
	bent_world_t* world,
	bent_sys_reg_t listener,
	ssync_control_callback_fn_t callback
);

void
ssync_bent_sync_static_schema(bent_world_t* world, const ssync_static_schema_t* schema);

void
(ssync_prop_asset)(ssync_ctx_t* ctx, const void** asset);

void
ssync_attach_snet(bent_world_t* world, struct snet_s* snet);

AUTOLIST_DECLARE(ssync__comps)

#endif
