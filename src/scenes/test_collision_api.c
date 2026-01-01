#include <bgame/scene.h>
#include <bgame/reloadable.h>
#include <bgame/asset/collision_shape.h>
#include <cute.h>
#include <dcimgui.h>

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(test_spatial_hash, TYPE, NAME)

SCENE_VAR(bgame_collision_shape_t, shape_a)
SCENE_VAR(bgame_collision_shape_t, shape_b)
SCENE_VAR(CF_V2, translation_a)
SCENE_VAR(CF_V2, translation_b)
SCENE_VAR(float, rotation_a)
SCENE_VAR(float, rotation_b)

static const char* shape_types[] = {
	"None",
	"Circle",
	"Box",
	"Capsule",
	"Poly"
};

static const CF_V2 verts[] = {
	{
		-33.710567474365234,
		40.9147834777832
	},
	{
		23.421588897705078,
		41.243465423583984
	},
	{
		50.009151458740234,
		1.202066421508789
	},
	{
		34.859615325927734,
		-33.38836669921875
	},
	{
		11.412944793701172,
		-28.82425880432129
	},
	{
		-20.357158660888672,
		-41.745704650878906
	},
	{
		-50.074703216552734,
		-11.264490127563477
	}
};

static void
shape_ui(
	const char* title,
	bgame_collision_shape_t* shape,
	CF_V2* translation,
	float* rotation
) {
	float width = cf_app_get_width();
	if (ImGui_Begin(title, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui_SliderFloat2("translation", &translation->x, -width * 0.5f, width * 0.5f);
		ImGui_SliderFloat("rotation", rotation, 0.f, CF_PI * 2.f);

		int type = shape->type;
		if (ImGui_ComboCharEx("Type", &type, shape_types, CF_ARRAY_SIZE(shape_types), -1)) {
			shape->type = type;
		}

		switch (shape->type) {
			case CF_SHAPE_TYPE_NONE:
				break;
			case CF_SHAPE_TYPE_CIRCLE:
				ImGui_SliderFloat("Radius", &shape->data.circle.r, 0.f, 1000.f);
				break;
			case CF_SHAPE_TYPE_AABB:
				break;
			case CF_SHAPE_TYPE_CAPSULE:
				break;
			case CF_SHAPE_TYPE_POLY: {
				memcpy(shape->data.poly.verts, verts, sizeof(verts));
				shape->data.poly.count = CF_ARRAY_SIZE(verts);
				cf_make_poly(&shape->data.poly);
			} break;
		}
	}
	ImGui_End();
}

static void
draw_shape(
	const bgame_collision_shape_t* shape,
	CF_V2 translation,
	float rotation
) {
	cf_draw_push();
	cf_draw_translate_v2(translation);
	cf_draw_rotate(rotation);

	switch (shape->type) {
		case CF_SHAPE_TYPE_NONE:
			break;
		case CF_SHAPE_TYPE_CIRCLE:
			cf_draw_circle(shape->data.circle, 0.2f);
			break;
		case CF_SHAPE_TYPE_AABB:
			cf_draw_box(shape->data.aabb, 0.2f, 0.2f);
			break;
		case CF_SHAPE_TYPE_CAPSULE:
			cf_draw_capsule(shape->data.capsule, 0.2f);
			break;
		case CF_SHAPE_TYPE_POLY: {
			const CF_Poly* poly = &shape->data.poly;

			cf_draw_polyline(poly->verts, poly->count, 0.2f, true);

			for (int i = 0; i < poly->count; ++i) {
				CF_V2 point_a = poly->verts[i];
				CF_V2 point_b = poly->verts[(i + 1) % poly->count];
				CF_V2 midpoint = cf_mul(cf_add(point_a, point_b), 0.5f);
				cf_draw_arrow(
					midpoint,
					cf_add(midpoint, cf_mul(poly->norms[i], 10.f)),
					0.2f,
					1.5f
				);
			}
		} break;
	}

	cf_draw_pop();
}

static bgame_collision_shape_t
transform_shape(
	const bgame_collision_shape_t* shape,
	CF_Transform transform
) {
	bgame_collision_shape_t result = {
		.type = CF_SHAPE_TYPE_POLY,
		.data.poly.count = shape->data.poly.count,
	};
	for (int i = 0 ; i < shape->data.poly.count; ++i) {
		result.data.poly.verts[i] = cf_mul(transform, shape->data.poly.verts[i]);
	}
	cf_norms(result.data.poly.verts, result.data.poly.norms, shape->data.poly.count);
	return result;
}

static void
update(void) {
	cf_app_update(NULL);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);

	shape_ui("Shape A", &shape_a, &translation_a, &rotation_a);
	shape_ui("Shape B", &shape_b, &translation_b, &rotation_b);

	CF_Transform transform_a = cf_make_transform_TR(translation_a, rotation_a);
	CF_Transform transform_b = cf_make_transform_TR(translation_b, rotation_b);

	bgame_collision_shape_t shape_a_transformed = transform_shape(&shape_a, transform_a);
	bgame_collision_shape_t shape_b_transformed = transform_shape(&shape_b, transform_b);

	draw_shape(&shape_a_transformed, (CF_V2){ 0 }, 0.f);
	draw_shape(&shape_b_transformed, (CF_V2){ 0 }, 0.f);

	CF_Manifold manifold = { 0 };
	cf_collide(
		&shape_a_transformed.data, NULL, shape_a_transformed.type,
		&shape_b_transformed.data, NULL, shape_b_transformed.type,
		&manifold
	);
	if (manifold.count > 0) {
		cf_draw_push_color(cf_color_red());
		for (int i = 0; i < manifold.count; ++i) {
			cf_draw_circle2(manifold.contact_points[i], 3.f, 1.f);
			cf_draw_arrow(
				manifold.contact_points[i],
				cf_add(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
				0.1f,
				3.f
			);
			cf_draw_push_color(cf_color_green());
			cf_draw_arrow(
				manifold.contact_points[i],
				cf_sub(manifold.contact_points[i], cf_mul(manifold.n, 20.f)),
				0.1f,
				3.f
			);
			cf_draw_pop_color();
		}
		cf_draw_pop_color();
	}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(test_collision_api) = {
	.update = update,
};
