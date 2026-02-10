#include <bgame/entrypoint.h>
#include <bgame/allocator.h>
#include <bgame/allocator/tracked.h>
#include <bgame/scene.h>
#include <blog.h>
#include <cute.h>
#include <barena.h>
#include <bgame/asset.h>
#include <slopnet.h>
#include <stdlib.h>
#include <slopsync/schema.h>
#include "slopsync.h"
#include "ssync_schema.h"
#include "globals.h"
#include "fs.h"

#if BGAME_RELOADABLE
#include <bent.h>
#endif

static const char* WINDOW_TITLE = "Net asteroid";
BGAME_VAR(bool, app_created) = false;
BGAME_VAR(bgame_asset_bundle_t*, predefined_assets) = { 0 };

static void
load_assets(void) {
	bgame_asset_init(&predefined_assets, bgame_default_allocator);
	BGAME_FOREACH_DEFINED_ASSET(asset) {
		// Optional tag filtering
		bgame_asset_load_def(predefined_assets, asset);
	}
}

static void
snet_log(const char* fmt, va_list args, void* logctx) {
	blog_vwrite(BLOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, args);
}

static void
init(int argc, const char** argv) {
	// Cute Framework
	if (!app_created) {
		BLOG_INFO("Creating app");
		int options =
			  CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT
			| CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT_BIT;
		CF_Result result = cf_make_app(WINDOW_TITLE, 0, 0, 0, 1280, 720, options, argv[0]);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_FATAL("Could not create app: %s", result.details);
			abort();
		}

		// Mount assets dir
		result = cf_fs_mount("./assets", "/assets", true);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_WARN("Could not mount %s: %s", ".", result.details);
		}
#if BGAME_RELOADABLE
		cf_fs_mount("./src", "/assets", true);
#endif

		cf_app_init_imgui();
		init_fs();

		const char* user_dir = cf_fs_get_user_directory(
			"bullno1",
			getenv("GAME_NAME") != NULL ? getenv("GAME_NAME") : "asteroid-net"
		);
		cf_fs_set_write_directory(user_dir);
		cf_fs_mount(user_dir, "/user", true);

		size_t cookie_size;
		char* cookie = cf_fs_read_entire_file_to_memory_and_nul_terminate("/user/cookie", &cookie_size);
		if (cookie != NULL && cookie_size > 0) {
			g_saved_snet_cookie = cookie;
		}

		g_snet = snet_init(&(snet_config_t){
			.host = "snet-dev.bullno1.com",
			.path = "/snet",
			.log = snet_log,
		});

		app_created = true;
	}

	cf_set_fixed_timestep(30);
	cf_app_set_vsync(true);
	cf_app_set_title(WINDOW_TITLE);

	load_assets();

#if BGAME_RELOADABLE
	bent_world_t* tmp_world = NULL;
	bent_init(&tmp_world, bgame_default_allocator);
	ssync_sync_static_schema(ssync_client(tmp_world), &ssync_schema);
	bent_cleanup(&tmp_world);
#endif

	if (bgame_current_scene() == NULL) {
		bgame_push_scene("main_menu");
		bgame_scene_update();
	}
}

static void
report_allocator_stats(
	const char* name,
	bgame_allocator_stats_t stats,
	void* userdata
) {
	BLOG_DEBUG("%s: Total %zu, Peak %zu", name, stats.total, stats.peak);
}

static void
cleanup(void) {
	bgame_clear_scene_stack();
	bgame_asset_cleanup(&predefined_assets);
	snet_cleanup(g_snet);
	cf_destroy_app();

	BLOG_DEBUG("--- Allocator stats ---");
	bgame_enumerate_tracked_allocators(report_allocator_stats, NULL);
}

static void
update(void) {
	bgame_asset_check_bundle(predefined_assets);
	snet_update(g_snet);
	bgame_scene_update();
}

static void
after_reload(void) {
	load_assets();
	bgame_scene_after_reload();
}

static bgame_app_t app = {
	.init = init,
	.cleanup = cleanup,
	.update = update,
	.before_reload = bgame_scene_before_reload,
	.after_reload = after_reload,
};

BGAME_ENTRYPOINT(app)
