#include "assets.h"

BGAME_DEFINE_SPRITE(spr_player_ship) = { .path = "/assets/sprites/player_ship_blue.ase" };
BGAME_DEFINE_SPRITE(spr_asteroid_big_1) = { .path = "/assets/sprites/asteroid_big1.png" };
BGAME_DEFINE_SPRITE(spr_exhaust) = { .path = "/assets/sprites/fire.ase" };
BGAME_DEFINE_SPRITE(spr_friendly_projectile) = { .path = "/assets/sprites/friendly_projectile.png" };
BGAME_DEFINE_SPRITE(spr_muzzle_flash) = { .path = "/assets/sprites/muzzle-flash.png" };

BGAME_DEFINE_COLLISION_SHAPE(shape_asteroid_big_1) = { .path = "/assets/shapes/meteor-big1.json" };
BGAME_DEFINE_COLLISION_SHAPE(shape_player_ship) = { .path = "/assets/shapes/player-ship.json" };

BGAME_DEFINE_IMAGE(img_background_stars) = { .path = "/assets/backgrounds/blue.png" };

BGAME_DEFINE_FONT(font_ui_label) = { .path = "/assets/fonts/kenvector_future.ttf" };
