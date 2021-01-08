#version 120

varying vec4 color;
varying vec2 texCoord;
varying vec4 normal;
varying vec4 frag_pos;
varying float f_use_texture;
varying float f_is_shadow;
varying vec4 f_light_position;
varying vec4 cam_pos;

uniform sampler2D texture;

void main()
{

	if (f_is_shadow == 0) {
	// ------------------------------------------------------------------
	// ----- GET BASE COLOR USING EITHER COLOR ATTRIBUTE OR TEXTURE -----
	// ------------------------------------------------------------------

	vec4 real_color = color;

	if (f_use_texture == 1.0) {
		real_color = texture2D(texture, texCoord);
	}

	//real_color = normal;
	//real_color = f_light_position;



	// -------------------------
	// ----- AMBIENT LIGHT -----
	// -------------------------

	vec4 ambient = real_color * 0.4;



	// -------------------
	// ----- DIFFUSE -----
	// -------------------

	vec4 light_direction = normalize(f_light_position - frag_pos);
	float diff = max(dot(normalize(normal), light_direction), 0.0);
	vec4 diffuse = diff * real_color;

	// --------------------
	// ----- SPECULAR -----
	// --------------------

	vec4 camera_angle = normalize(cam_pos - frag_pos);
	vec4 bounce_angle = reflect(-light_direction, normalize(normal));
	float spec = pow(max(dot(camera_angle, bounce_angle), 0.0), 256);
	vec4 specular = 0.5 * spec * real_color;


	vec4 result = ambient + diffuse + specular;

	result.w = 1.0;
	gl_FragColor = result;
	}




	// SHADOWS
	else {
		gl_FragColor = vec4(0.0, 0.4, 0.0, 1.0);
		
	}
}
