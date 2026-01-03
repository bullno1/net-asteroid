layout (set = 2, binding = 1) uniform sampler2D tex_background;
layout (set = 3, binding = 1) uniform shd_uniforms {
	vec2 u_background_size;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	vec2 uv = fract(screen_uv * u_background_size / textureSize(tex_background, 0));
	return texture(tex_background, uv);
}
