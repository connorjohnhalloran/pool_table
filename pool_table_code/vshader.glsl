#version 120

attribute vec4 vPosition;
attribute vec4 vColor;
attribute vec2 vTexCoord;
attribute vec4 vNormal;

varying vec4 color;
varying vec2 texCoord;
varying vec4 normal;
varying vec4 frag_pos;
varying float f_use_texture;
varying float f_is_shadow;
varying vec4 f_light_position;
varying vec4 cam_pos;

uniform mat4 ctm;
uniform mat4 ball_xform;
uniform int use_texture;
uniform vec4 lightbulb_position;
uniform int is_shadow;

void main()
{
	color = vColor;
	texCoord = vTexCoord;
	normal = ctm * ball_xform * vNormal;
	frag_pos = ctm * vPosition;
	f_use_texture = use_texture;
	f_light_position = ctm * lightbulb_position;
	cam_pos = -ctm * vec4(0.0f, 0.0f, 0.0f, 1.0);
	f_is_shadow = is_shadow;


	mat4 flat = mat4(1.0, 0.0, 0.0, 0.0,
			 0.0, 1.0, 0.0, 0.0,
			 0.0, 0.0, 0.01, 0.0,
			 0.0, 0.0, 0.0, 1.0);

	if (is_shadow == 0) {
		gl_Position = flat * ctm * ball_xform * vPosition;
	} else {

	mat4 scale_down = mat4(1.0, 0.0, 0.0, 0.0,
			 0.0, 0.1, 0.0, 0.0,
			 0.0, 0.0, 1.0, 0.0,
			 0.0, 0.0, 0.0, 1.0);
		gl_Position = flat * ctm * scale_down * ball_xform *  vPosition;
	}
}
