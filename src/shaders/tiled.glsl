layout (set = 2, binding = 1) uniform sampler2D tex_background;
layout (set = 3, binding = 1) uniform shd_uniforms {
	vec2 u_background_size;
	vec2 u_offset;
	float u_scale;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	vec2 texture_size = textureSize(tex_background, 0);
	vec2 uv = fract((screen_uv * u_background_size + u_offset) * u_scale / texture_size);
	vec2 smoothed_uv = smooth_uv(uv, u_texture_size);
	return texture(tex_background, smoothed_uv);
}
