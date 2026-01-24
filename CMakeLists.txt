cmake_minimum_required(VERSION 3.15)
project(net-asteroid)

option(RELOADABLE "Is the program reloadable" ON)

# Fix output dir
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>-$<IF:$<BOOL:${RELOADABLE}>,reloadable,static>$<$<PLATFORM_ID:Emscripten>:-web>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>-$<IF:$<BOOL:${RELOADABLE}>,reloadable,static>$<$<PLATFORM_ID:Emscripten>:-web>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>-$<IF:$<BOOL:${RELOADABLE}>,reloadable,static>$<$<PLATFORM_ID:Emscripten>:-web>)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
if (IPO_SUPPORTED AND (NOT RELOADABLE))
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif ()

# Add deps first

add_subdirectory(deps)
set_target_properties(
	cute PROPERTIES
    # some properties that successfully get applied here
    LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_BINARY_DIR}/bin/$<CONFIG>-$<IF:$<BOOL:${RELOADABLE}>,reloadable,static>
)

# Then everything else to avoid leaking into deps

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

function (setup_library LIBRARY_NAME TYPE SOURCES)
	if (${TYPE} STREQUAL "SHARED")
		string(REPLACE "-" "_" LIBRARY_NAME_NO_SLUG ${LIBRARY_NAME})
		string(TOUPPER ${LIBRARY_NAME_NO_SLUG} LIBRARY_NAME_UPPER)
		add_library(${LIBRARY_NAME} SHARED ${SOURCES})
		target_compile_definitions(${LIBRARY_NAME} PUBLIC ${LIBRARY_NAME_UPPER}_SHARED)
		target_compile_definitions(${LIBRARY_NAME} PRIVATE ${LIBRARY_NAME_UPPER}_BUILD)
		set_target_properties(${LIBRARY_NAME} PROPERTIES C_VISIBILITY_PRESET hidden)

		if (NOT MSVC)
			target_link_options(${LIBRARY_NAME} PRIVATE
				-Wl,--exclude-libs,ALL
				-Wl,--no-whole-archive
			)
		endif ()
	else ()
		add_library(${LIBRARY_NAME} STATIC ${SOURCES})
	endif ()
endfunction ()

function (add_bgame_app NAME SOURCES)
	if (RELOADABLE)
		add_library(${NAME} SHARED ${SOURCES})
		set_target_properties(${NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
		target_link_libraries(${NAME} PRIVATE bgame)
		if (NOT MSVC)
			target_link_options(${NAME} PRIVATE
				-Wl,--exclude-libs,ALL
				-Wl,--no-whole-archive
				-Wl,--no-undefined
			)
		endif ()
		set_target_properties(${NAME} PROPERTIES
			OUTPUT_NAME "${NAME}"
			PREFIX ""
		)

		add_executable(${NAME}-loader "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bgame/src/loader_stub.c")
		target_link_libraries(${NAME}-loader PRIVATE bgame-loader)
		set_target_properties(${NAME}-loader PROPERTIES
			OUTPUT_NAME "${NAME}"
			PREFIX ""
		)
	else ()
		add_executable(${NAME} ${SOURCES})
		target_link_libraries(${NAME} PRIVATE bgame bgame-loader bgame-loader-stub)
	endif ()
endfunction ()

if (MSVC)
	# 4200: Flexible array member is a standard feature since C99
	add_compile_options(/W4 /WX /wd4200)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

	# Because CMake can't set proper flags even with CMAKE_INTERPROCEDURAL_OPTIMIZATION_Release
	if (NOT RELOADABLE)
		set(RELEASE_COMPILER_FLAGS "/Gy;/Ob2;/GL;/Oi")
		set(RELEASE_LINKER_FLAGS "/LTCG")

		add_compile_options("$<$<CONFIG:RelWithDebInfo>:${RELEASE_COMPILER_FLAGS}>")
		add_link_options("$<$<CONFIG:RelWithDebInfo>:${RELEASE_LINKER_FLAGS}>")
	endif ()
else()
	add_compile_options(
		-Wall -Wextra -pedantic -Werror
		-Wno-unused-parameter
		-Wno-overlength-strings
		-Wno-error=c23-extensions
	)
endif()

add_subdirectory(bgame)
add_subdirectory(src)
