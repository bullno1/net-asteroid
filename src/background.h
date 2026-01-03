#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <cute_graphics.h>
#include <cute_image.h>

typedef struct {
	CF_Texture texture;
	CF_Image* source_image;
	int image_version;
} background_t;

void
init_background(background_t* background, CF_Image* image);

void
cleanup_background(background_t* background);

void
draw_background(
	background_t* background,
	CF_Aabb box,
	CF_V2 offset,
	float scale
);

#endif
