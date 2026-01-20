#ifndef SLOPSYNC_BENT_INTEGRATION_H
#define SLOPSYNC_BENT_INTEGRATION_H

#include <slopsync/client.h>
#include <bent.h>

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

const ssync_t*
ssync_bent_client(bent_world_t* world);

const ssync_info_t*
ssync_bent_info(bent_world_t* world);

const ssync_obj_info_t*
ssync_bent_entity_info(bent_world_t* world, bent_t ent);

static inline void
ssync_bent_track(bent_world_t* world, bent_t ent, ssync_obj_flags_t flags) {
	bent_add_comp_slopsync_link(world, ent, &(slopsync_link_t){ .flags = flags });
}

#endif
