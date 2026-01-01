set(SOURCES
	"main.c"
	"libs.c"

	"systems/snap_transform.c"
	"systems/sprite.c"
	"systems/wrap_around.c"
	"systems/linear_motion.c"
	"systems/collision.c"
	"systems/collider_maker.c"
	"systems/collision_response.c"
	"systems/debug_control.c"
	"ecs.c"

	"spatial_hash.c"

	"scenes/test_spatial_hash.c"
	"scenes/test_collision_api.c"
)
add_bgame_app(net-asteroid "${SOURCES}")
