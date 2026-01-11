#include <bgame/scene.h>
#include <bgame/allocator.h>
#include <bgame/allocator/tracked.h>
#include <bgame/allocator/frame.h>
#include <bgame/reloadable.h>
#include <bgame/ui.h>
#include <bgame/ui/rich_text.h>
#include <cute.h>
#include <slopnet.h>
#include <barena.h>
#include "../globals.h"
#include "../templates.h"
#include "../ui.h"
#include "../background.h"
#include "../assets.h"
#include "../ecs.h"
#include "../fs.h"

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main_menu, TYPE, NAME)

BGAME_DECLARE_SCENE_ALLOCATOR(main_menu)

typedef enum {
	MENU_STATE_ROOT,
	MENU_STATE_MULTIPLAYER,
} menu_state_T;

SCENE_VAR(bent_world_t*, world)
SCENE_VAR(background_t, background)
SCENE_VAR(menu_state_T, menu_state)
SCENE_VAR(barena_t, game_list_arena)
SCENE_VAR(snet_game_info_t*, game_list)
SCENE_VAR(int, num_games)

static CF_Rnd rnd = { 0 };

static void
init(void) {
	if (bent_init(&world, scene_allocator)) {
		rnd = cf_rnd_seed(CF_TICKS);
		menu_state = MENU_STATE_ROOT;

		for (int i = 0; i < 10; ++i) {
			create_asteroid(world, &rnd);
		}

		barena_init(&game_list_arena, bgame_arena_pool);
	}

	init_background(&background, img_background_stars);
	load_ui_fonts();
}

static void
cleanup(void) {
	barena_reset(&game_list_arena);
	bent_cleanup(&world);
}

static void
fixed_update(void* userdata) {
	ecs_update_fixed(world);
}

static bool
menu_entry(const char* text) {
	bool clicked = false;

	BUI(
		// Local id so it doesn't clash
		CLAY_SID_LOCAL(((Clay_String){ .chars = text, .length = strlen(text) })),
		{ .layout.sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) } }
	) {
		bool hovered = Clay_Hovered();
		const char* label = bgame_fmt(
			hovered ? "<wave>%s</wave>" : "%s",
			text
		);
		bgame_ui_rich_text(CLAY_ID_LOCAL("Label"), (bgame_ui_rich_text_t){
			.sizing = BGAME_UI_RICH_TEXT_GROW,
			.text = label,
			.font_id = FONT_UI_LABEL,
			.font_size = 40,
			.text_len = -1,
			.color = bgame_ui_color_from_cf(cf_color_white()),
		});

		clicked = hovered && cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT);
	}

	return clicked;
}

static void
root_menu(void) {
	BUI(CLAY_ID("TopSpacer"), {
		.layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
	}) {
	}

	BUI(CLAY_ID("MenuButtons"), {
		.layout = {
			.padding = {
				.top = 10,
				.bottom = 50,
			},
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.childAlignment = {
				.x = CLAY_ALIGN_X_CENTER,
			},
			.childGap = 20,
		}
	}) {
		if (menu_entry("Single player")) {
			bgame_push_scene("main_game");
		}

		if (menu_entry("Multiplayer")) {
			menu_state = MENU_STATE_MULTIPLAYER;

			if (snet_auth_state(g_snet) == SNET_UNAUTHORIZED && g_saved_snet_cookie) {
				snet_login_with_cookie(g_snet, (snet_blob_t){
					.ptr = g_saved_snet_cookie,
					.size = strlen(g_saved_snet_cookie),
				});
			}
		}

		if (menu_entry("Quit")) {
		}
	}
}

static void
multiplayer_menu(void) {
	BUI(CLAY_ID("TopSpacer"), {
		.layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
	}) {
	}

	BUI(CLAY_ID("MenuButtons"), {
		.layout = {
			.padding = {
				.top = 10,
				.bottom = 50,
			},
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.childAlignment = {
				.x = CLAY_ALIGN_X_CENTER,
			},
			.childGap = 20,
		}
	}) {
		snet_auth_state_t auth_state = snet_auth_state(g_snet);

		switch (auth_state) {
			case SNET_UNAUTHORIZED: {
				if (menu_entry("Login with itch.io")) {
					snet_login_with_itchio(g_snet);
				}
			} break;
			case SNET_AUTHORIZING: {
				menu_entry("Logging in...");
			} break;
			case SNET_AUTHORIZED: {
				snet_lobby_state_t lobby_state = snet_lobby_state(g_snet);
				switch (lobby_state) {
					case SNET_LISTING_GAMES:
					case SNET_IN_LOBBY: {
						for (int i = 0; i < num_games; ++i) {
							const char* game_name = bgame_fmt("%.*s", (int)game_list[i].creator.size, (const char*)game_list[i].creator.ptr);
							if (menu_entry(game_name)) {
								snet_join_game(g_snet, game_list[i].join_token);
							}
						}

						if (
							lobby_state != SNET_LISTING_GAMES
							&&
							menu_entry("Refresh game list")
						) {
							snet_list_games(g_snet);
						}

						BUI(CLAY_ID_LOCAL("Separator"), {
							.layout = {
								.padding = {
									.top = 10,
									.bottom = 50,
								},
								.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
							},
							.border = {
								.color = bgame_ui_color_from_cf(cf_color_white()),
								.width.bottom = 1,
							}
						}) {
						}

						if (menu_entry("Create game")) {
							snet_create_game(g_snet, &(snet_game_options_t){
								.visibility = SNET_GAME_PUBLIC,
								.max_num_players = 2,
							});
						}
					} break;
					case SNET_CREATING_GAME: {
						menu_entry("Creating game...");
					} break;
					case SNET_JOINING_GAME: {
						menu_entry("Joining game...");
					} break;
					case SNET_JOINED_GAME: {
					} break;
				}
			} break;
		}

		if (menu_entry("Back")) {
			menu_state = MENU_STATE_ROOT;
		}
	}
}

static snet_blob_t
copy_snet_blob(snet_blob_t blob) {
	snet_blob_t copy = {
		.size = blob.size,
		.ptr = barena_memalign(&game_list_arena, blob.size, 1),
	};
	memcpy((void*)copy.ptr, blob.ptr, blob.size);
	return copy;
}

static void
update(void) {
	cf_app_update(fixed_update);
	ecs_update_variable(world);
	ecs_render(world);

	// slopnet
	{
		const snet_event_t* event;
		while ((event = snet_next_event(g_snet)) != NULL) {
			if (event->type == SNET_EVENT_LOGIN_FINISHED) {
				if (event->login.status == SNET_OK) {
					BLOG_INFO("Logged in with token: " SNET_BLOB_FMT "\n", SNET_BLOB_FMT_ARGS(event->login.data));
					cf_fs_write_string_range_to_file("/cookie", (char*)event->login.data.ptr, (char*)event->login.data.ptr + event->login.data.size);
					sync_fs();
					snet_list_games(g_snet);
				} else if (event->login.status == SNET_ERR_IO) {
					BLOG_INFO("Network error\n");
				} else if (event->login.status == SNET_ERR_REJECTED) {
					BLOG_INFO("Logged failed with reason: " SNET_BLOB_FMT "\n", SNET_BLOB_FMT_ARGS(event->login.data));
				}
			} else if (event->type == SNET_EVENT_LIST_GAMES_FINISHED) {
				if (event->list_games.status == SNET_OK) {
					barena_reset(&game_list_arena);
					num_games = event->list_games.num_games;
					game_list = barena_memalign(
						&game_list_arena,
						sizeof(snet_game_info_t) * num_games,
						_Alignof(snet_game_info_t)
					);
					for (int i = 0; i < num_games; ++i) {
						game_list[i].creator = copy_snet_blob(event->list_games.games[i].creator);
						game_list[i].join_token = copy_snet_blob(event->list_games.games[i].join_token);
					}
				}
			} else if (event->type == SNET_EVENT_CREATE_GAME_FINISHED) {
				if (event->create_game.status == SNET_OK) {
					snet_join_game(g_snet, event->create_game.info.join_token);
				}
			} else if (event->type == SNET_EVENT_JOIN_GAME_FINISHED) {
				if (event->join_game.status == SNET_OK) {
					bgame_push_scene("main_game");
				}
			}
		}
	}

	// UI
	{
		bgame_update_ui();
		Clay_BeginLayout();
		BUI(CLAY_ID("Root"), {
			.layout = {
				.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
				.childAlignment = {
					.x = CLAY_ALIGN_X_CENTER,
				},
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.padding = CLAY_PADDING_ALL(10),
			},
		}) {
			switch (menu_state) {
				case MENU_STATE_ROOT:
					root_menu();
					break;
				case MENU_STATE_MULTIPLAYER:
					multiplayer_menu();
					break;
			}
		}
		Clay_RenderCommandArray ui_commands = Clay_EndLayout();
		cf_draw_push_layer(DRAW_LAYER_UI);
		bgame_render_ui(ui_commands);
		cf_draw_pop_layer();
	}

	// Background
	{
		float width = cf_app_get_width();
		float height = cf_app_get_height();
		float half_width = width * 0.5f;
		float half_height = height * 0.5f;
		CF_Aabb screen = {
			.min = { -half_width, -half_height },
			.max = { +half_width, +half_height },
		};
		cf_draw_push_layer(DRAW_LAYER_BACKGROUND);
		CF_V2 mouse = { cf_mouse_x(), cf_mouse_y() };
		draw_background(&background, screen, cf_mul(mouse, 0.1f), 1.f);
		cf_draw_pop_layer();
	}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main_menu) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
