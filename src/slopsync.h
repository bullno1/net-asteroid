#ifndef SLOPSYNC_BENT_INTEGRATION_H
#define SLOPSYNC_BENT_INTEGRATION_H

#include <slopsync/client.h>
#include <bent.h>

typedef struct {
	const char* filename;
	uint64_t hash;
	size_t size;
	const uint8_t* content;
} ssync_static_schema_t;

void
ssync_bent_sync_static_schema(bent_world_t* world, const ssync_static_schema_t* schema);

#endif
