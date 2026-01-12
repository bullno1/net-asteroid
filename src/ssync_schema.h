#pragma once

#include "slopsync.h"

static const uint8_t ssync_schema_content[14] = {
	0x00,0x01,0x00,0x03,0x00,0x01,0x06,0x0A,0x01,0x06,0x12,0x01,0x06,0x42,
};

static const ssync_static_schema_t ssync_schema = {
	.filename = __FILE__,
	.size = 14ull,
	.hash = 2041607168103241657ull,
	.content = ssync_schema_content,
};
