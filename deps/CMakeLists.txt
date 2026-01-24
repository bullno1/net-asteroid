# --- cute_framework ---
if (${RELOADABLE})
	set(CF_FRAMEWORK_STATIC OFF CACHE BOOL "")
else ()
	set(CF_FRAMEWORK_STATIC ON CACHE BOOL "")
endif ()
set(CF_FRAMEWORK_BUILD_SAMPLES OFF CACHE BOOL "")
set(CF_FRAMEWORK_BUILD_TESTS OFF CACHE BOOL "")
set(CF_RUNTIME_SHADER_COMPILATION OFF CACHE BOOL "")

add_subdirectory("cute_framework" EXCLUDE_FROM_ALL)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	if (TARGET SDL3-shared)
		get_target_property(current_link_options SDL3-shared LINK_OPTIONS)
		list(REMOVE_ITEM current_link_options "-Wl,--no-undefined")
		target_link_options(SDL3-shared PRIVATE "${current_link_options}")
		set_target_properties(SDL3-shared PROPERTIES LINK_OPTIONS "${current_link_options}")
	endif ()
endif ()

# Make the include path system so that targets linking to cute framework can
# use a stricter warning settings.
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/include")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries/cimgui")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries/cimgui/imgui")

# --- remodule --
add_library(remodule INTERFACE)
target_include_directories(remodule INTERFACE remodule)

# --- blibs --
add_library(blibs INTERFACE)
target_include_directories(blibs INTERFACE blibs)

# --- slopnet ---
set(SLOPNET_BUILD_CF OFF CACHE BOOL "")
add_subdirectory("slopnet" EXCLUDE_FROM_ALL)

# --- slopsync ---
set(SLOPSYNC_BUILD_DEPS OFF CACHE BOOL "")
add_subdirectory("slopsync" EXCLUDE_FROM_ALL)
