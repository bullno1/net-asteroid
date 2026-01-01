set(SOURCES
	"main.c"
	"libs.c"

	"systems/snap_transform.c"
	"systems/sprite.c"
	"systems/wrap_around.c"
	"systems/collision.c"
	"systems/collider_maker.c"
	"systems/debug_control.c"
	"ecs.c"
	"asteroid.c"

	"spatial_hash.c"

	"scenes/test_spatial_hash.c"
)
add_bgame_app(net-asteroid "${SOURCES}")
