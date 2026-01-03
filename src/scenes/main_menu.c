#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/allocator/frame.h>
#include <bgame/reloadable.h>
#include <bgame/ui.h>
#include <bgame/ui/rich_text.h>
#include <cute.h>
#include "../ui.h"
#include "../background.h"
#include "../assets.h"
#include "../ecs.h"

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main_menu, TYPE, NAME)

BGAME_DECLARE_SCENE_ALLOCATOR(main_menu)

SCENE_VAR(bent_world_t*, world)
SCENE_VAR(background_t, background)

/*static CF_Rnd rnd = { 0 };*/

static void
init(void) {
	if (bent_init(&world, scene_allocator)) {
	}

	init_background(&background, img_background_stars);
	load_ui_fonts();
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
update(void) {
	cf_app_update(fixed_update);
	ecs_update_variable(world);
	ecs_render(world);

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

				menu_entry("Multiplayer");

				if (menu_entry("Quit")) {
				}
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

static void
cleanup(void) {
	bent_cleanup(&world);
}

BGAME_SCENE(main_menu) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
