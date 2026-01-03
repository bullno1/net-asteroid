set(SOURCES
	"main.c"
	"libs.c"

	"systems/snap_transform.c"
	"systems/sprite.c"
	"systems/wrap_around.c"
	"systems/offscreen_cull.c"
	"systems/linear_motion.c"
	"systems/collision.c"
	"systems/collider_maker.c"
	"systems/collision_response.c"
	"systems/ship_controller.c"
	"systems/ship_render.c"
	"systems/debug_control.c"
	"ecs.c"
	"assets.c"

	"spatial_hash.c"

	"scenes/test_spatial_hash.c"
	"scenes/test_collision_api.c"
)
add_bgame_app(net-asteroid "${SOURCES}")

if (EMSCRIPTEN)
	set_target_properties(net-asteroid PROPERTIES SUFFIX ".html")
	target_compile_options(net-asteroid PRIVATE -fno-rtti -fno-exceptions -gsplit-dwarf)
	target_link_options(net-asteroid PRIVATE
		-sASYNCIFY=1
		-sMALLOC=emmalloc-debug
		-gseparate-dwarf
		--preload-file "${CMAKE_CURRENT_LIST_DIR}/../assets@/assets"
	)
endif()
