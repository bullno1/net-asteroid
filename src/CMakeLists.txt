set(SOURCES
	"main.c"
	"libs.c"

	"systems/snap_transform.c"
	"systems/sprite.c"
	"ecs.c"
	"asteroid.c"
	"wrap_around.c"

	"scenes/test_spatial_hash.c"
)
add_bgame_app(net-asteroid "${SOURCES}")
