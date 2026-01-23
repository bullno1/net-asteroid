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
	"background.c"
	"ecs.c"
	"ssync_comps.c"
	"slopsync.c"
	"globals.c"
	"fs.c"
	"assets.c"

	"spatial_hash.c"

	"scenes/main_game.c"
	"scenes/main_menu.c"
	"scenes/test_collision_api.c"
	"scenes/test_background.c"

	"gen/shaders/tiled.h"
)
add_bgame_app(net-asteroid "${SOURCES}")
target_link_libraries(net-asteroid PRIVATE slopnet slopsync-client slopsync-static-schema)

function (compile_shader INPUT VAR_NAME OUTPUT)
	if (NOT EMSCRIPTEN)
		add_custom_command(
			OUTPUT ${OUTPUT}
			COMMAND cute-shaderc
				-I${CMAKE_CURRENT_SOURCE_DIR}/shaders
				-type=draw
				-varname=${VAR_NAME}
				-oheader=${OUTPUT}
				${INPUT}
			DEPENDS ${INPUT} cute-shaderc
		)
	endif ()
endfunction ()

compile_shader(
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/tiled.glsl
	s_tiled_shd_bytecode
	${CMAKE_CURRENT_SOURCE_DIR}/gen/shaders/tiled.h
)

if (EMSCRIPTEN)
	add_custom_target(copy-emscripten-shell
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/emscripten ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/emscripten
	)
	add_library(cute-emscripten-shell INTERFACE)
	add_dependencies(cute-emscripten-shell copy-emscripten-shell)
	target_link_options(cute-emscripten-shell INTERFACE --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell.html")

	set_target_properties(net-asteroid PROPERTIES SUFFIX ".html")
	target_compile_options(net-asteroid PRIVATE -fno-rtti -fno-exceptions -gsplit-dwarf)
	target_link_libraries(net-asteroid PRIVATE cute-emscripten-shell)
	target_link_options(net-asteroid PRIVATE
		-sASYNCIFY=1
		-sMALLOC=emmalloc-debug
		-gseparate-dwarf
		-lidbfs.js
		--preload-file "${CMAKE_CURRENT_LIST_DIR}/../assets@/assets"
	)
endif()
