#include "slopsync.h"
#include <bent.h>
#include <bgame/allocator/frame.h>
#include <blog.h>
#include <string.h>
#include <errno.h>
#include "chibihash64.h"
#include "ecs.h"

typedef struct {
	ssync_t* ssync;
} sys_slopsync_t;

BENT_DEFINE_SYS(slopsync) = {
	.size = sizeof(sys_slopsync_t),
};

static bent_t
ssync_translate_id(sys_slopsync_t* sys, ssync_net_id_t net_id) {
	return (bent_t){ 0 };
}

static void
ssync_bent_sync(
	void* userdata,
	ssync_ctx_t* ctx,
	ssync_net_id_t obj_id
) {
	bent_world_t* world = userdata;
	sys_slopsync_t* sys = bent_get_sys_data(world, slopsync);
	bent_t ent = ssync_translate_id(sys, obj_id);

	if (ssync_prop_group(ctx, comp_transform.id)) {
		transform_t* transform = bent_get_comp_transform(world, ent);

		ssync_prop_float(ctx, &transform->current.translation.x, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_POSITION_X);
		ssync_prop_float(ctx, &transform->current.translation.y, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_POSITION_Y);
		ssync_prop_float(ctx, &transform->current.rotation, 3, SSYNC_PROP_INTERPOLATE | SSYNC_PROP_ROTATION);
	}
}

size_t
ssync_bent_schema_size(bent_world_t* world) {
	return ssync_schema_size(ssync_bent_sync, world);
}

void
ssync_bent_write_schema(bent_world_t* world, void* out, size_t out_size) {
	ssync_write_schema(ssync_bent_sync, world, out, out_size);
}

void
ssync_bent_sync_static_schema(bent_world_t* world, const ssync_static_schema_t* schema) {
	size_t size = ssync_schema_size(ssync_bent_sync, world);
	uint8_t* content = bgame_alloc_for_frame(size, _Alignof(uint8_t));
	ssync_write_schema(ssync_bent_sync, world, content, size);
	uint64_t hash = chibihash64(content, size, 0);

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

	fprintf(file, "static const uint8_t ssync_schema_content[%zu] = {", size);
	for (size_t i = 0; i < size; ++i) {
		if ((i % 16) == 0) {
			fprintf(file, "\n\t");
		}
		fprintf(file, "0x%02X,", content[i]);
	}
	fprintf(file, "\n};\n\n");

	fprintf(file, "static const ssync_static_schema_t ssync_schema = {\n");
	fprintf(file, "\t.filename = __FILE__,\n");
	fprintf(file, "\t.size = %zuull,\n", size);
	fprintf(file, "\t.hash = %zuull,\n", hash);
	fprintf(file, "\t.content = ssync_schema_content,\n");
	fprintf(file, "};\n");

	fclose(file);
}
