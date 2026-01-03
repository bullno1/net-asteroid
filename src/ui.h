#ifndef UI_H
#define UI_H

#include <bgame/ui.h>
#include "assets.h"

enum {
	FONT_DEFAULT,
	FONT_UI_LABEL,
};

static inline void
load_ui_fonts(void) {
	bgame_register_ui_font(FONT_UI_LABEL, font_ui_label->name);
}

#endif
