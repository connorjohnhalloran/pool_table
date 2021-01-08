/*
 * template.c
 *
 * An OpenGL source code template.
 */


#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#include <stdio.h>
#include <math.h>

#include "../mylib/initShader.h"
#include "../mylib/linear_alg.h"


#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

// -------------------------------------------------------------------------------- 

#define VERTS_PER_SPHERE 3072
#define TOTAL_VERTICES (VERTS_PER_SPHERE * 33)

#define bool int
#define true 1
#define false 0

// main places where information is stored
vec4 vertices[TOTAL_VERTICES];
vec4 orig_pos[TOTAL_VERTICES];
vec4 colors[TOTAL_VERTICES];
vec4 normals[TOTAL_VERTICES];
GLfloat tex_coords[TOTAL_VERTICES][2];
int id[TOTAL_VERTICES];
int num_vertices = TOTAL_VERTICES;

vec4 zero_vec = { 0.0f, 0.0f, 0.0f, 1.0f };
mat4 identity = {{1.0f, 0.0f, 0.0f, 0.0f},
	    {0.0f, 1.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 1.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f, 1.0f}};
mat4 ctm = {{1.0f, 0.0f, 0.0f, 0.0f},
	    {0.0f, 1.0f, 0.0f, 0.0f},
	    {0.0f, 0.0f, 1.0f, 0.0f},
	    {0.0f, 0.0f, 0.0f, 1.0f}};
GLuint ctm_location;

mat4 ball_xform;
GLuint ball_xform_location;

int use_texture;
GLuint use_texture_location;

int is_shadow;
GLuint is_shadow_location;

mat4 lightbulb_position;
GLuint lightbulb_location;
float lb_x = 0.0f;
float lb_y = 1.0f;
float lb_z = 0.0f;

// other important variables
int vtx_index = 0;

mat4 ball_start_mats[16];
mat4 ball_current_mats[16];
vec4 ball_start_pos[16];
vec4 ball_rolling_end_positions[16];
vec4 ball_direction_vectors[16];

#define FREE_CAM 0
#define BALL_LOCK_ON 1
int camera_state = FREE_CAM;

// generate all 16 pool balls
void generate_pool_balls();
void initialize_matricies();

void init(void)
{
    generate_pool_balls();
    initialize_matricies();
    for (int i = 0; i < TOTAL_VERTICES; ++i) {
	    orig_pos[i] = vertices[i];
    }

    GLubyte my_texels[512][512][3];

    FILE *fp;

    fp = fopen("pb_512_512.raw", "r");
    if (fp == NULL) {
	    printf("ERROR INVALID FILE");
	    exit(0);
    }

    fread(my_texels, 512 * 512 * 3, 1, fp);

    fclose(fp);



    // set ctm to identity matrix


    GLuint program = initShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    GLuint mytex;
    glGenTextures(1, &mytex);
    glBindTexture(GL_TEXTURE_2D, mytex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, my_texels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors) + sizeof(tex_coords) + sizeof(normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), sizeof(tex_coords), tex_coords);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors) + sizeof(tex_coords), sizeof(normals), normals);

    // POSITION
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    // COLOR
    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *) sizeof(vertices));

    // TEXTURE COORDINATES
    GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *) sizeof(vertices) + sizeof(colors));
    
    // NORMAL
    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *) sizeof(vertices) + sizeof(colors) + sizeof(tex_coords));

    // CTM
    ctm_location = glGetUniformLocation(program, "ctm");

    // BALL XFORM
    ball_xform_location = glGetUniformLocation(program, "ball_xform");

    // LIGHTBULB
    lightbulb_location = glGetUniformLocation(program, "lightbulb_position");

    // USE TEXTURE
    use_texture_location = glGetUniformLocation(program, "use_texture");
    
    is_shadow_location = glGetUniformLocation(program, "is_shadow");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDepthRange(1,0);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_LINE);

    glUniformMatrix4fv(ctm_location, 1, GL_FALSE, (GLfloat *) &ctm);
    glUniform4f(lightbulb_location, lb_x, lb_y, lb_z, 1.0f);




    // MAIN DRAWING LOGIC
    glUniform1i(use_texture_location, 1);
    glUniform1i(is_shadow_location, 0);

    ball_xform = identity;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);

    for (int i = 0; i < 16; ++i) {
	ball_xform = ball_current_mats[i];
    	glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    	glDrawArrays(GL_TRIANGLES, i * VERTS_PER_SPHERE, VERTS_PER_SPHERE);
    }

    /*
    // draw ball 1
    ball_xform = identity;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    glDrawArrays(GL_TRIANGLES, 0, VERTS_PER_SPHERE);

    ball_xform = identity;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    glDrawArrays(GL_TRIANGLES, VERTS_PER_SPHERE, 15*VERTS_PER_SPHERE);
    */

    // DRAW LIGHTBULB
    lightbulb_position = xform_trans_mat(lb_x, lb_y, lb_z);
    ball_xform = lightbulb_position;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    glUniform1i(use_texture_location, 0);
    glDrawArrays(GL_TRIANGLES, 16 * VERTS_PER_SPHERE, VERTS_PER_SPHERE);

    // DRAW TABLE
    ball_xform = identity;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    glDrawArrays(GL_TRIANGLES, 17 * VERTS_PER_SPHERE, 6);

	// DRAW SHADOWS
    glUniform1i(is_shadow_location, 1);
    ball_xform = identity;
    glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);

    for (int i = 0; i < 16; ++i) {
	ball_xform = ball_current_mats[i];
    	glUniformMatrix4fv(ball_xform_location, 1, GL_FALSE, (GLfloat *) &ball_xform);
    	glDrawArrays(GL_TRIANGLES, i * VERTS_PER_SPHERE, VERTS_PER_SPHERE);
    }


    glutSwapBuffers();
}

float horiz_pos = 0.0f;
float vert_pos = 0.5f;
float scale_factor = 0.8f;
int focused_ball = 0;

void keyboard(unsigned char key, int mousex, int mousey)
{

	// CAMERA MOVEMENT
    if(key == 'w') {
	    camera_state = FREE_CAM;
	    if(vert_pos + .1f < 3.14159f * 0.48f) {
		    vert_pos += .05f;
	    } 
    }
    if(key == 'a') {
	    camera_state = FREE_CAM;
	    horiz_pos -= .05f; 
    }
    if(key == 's') { 
	    camera_state = FREE_CAM;
	    if(vert_pos - .1f > 0.1f) { //.1
		    vert_pos -= .05f; 
	    } 
    }
    if(key == 'd') { 
	    camera_state = FREE_CAM;
	    horiz_pos += .05f; 
    }
    if(key == 'q') { 
	    camera_state = FREE_CAM;
	    scale_factor -= .05f; 
    }
    if(key == 'e') { 
	    camera_state = FREE_CAM;
	    scale_factor += .05f; 
    }

    // LIGHT MOVEMENT
    if (key == 'i') { lb_z -= 0.1f; }
    if (key == 'j') { lb_x -= 0.1f; }
    if (key == 'k') { lb_z += 0.1f; }
    if (key == 'l') { lb_x += 0.1f; }
    if (key == 'u') { lb_y += 0.1f; }
    if (key == 'o') { lb_y -= 0.1f; }

    // BALL FOCUSING
    if (key == '1') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 0;
    }
    if (key == '2') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 1;
    }
    if (key == '3') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 2;
    }
    if (key == '4') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 3;
    }
    if (key == '5') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 4;
    }
    if (key == '6') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 5;
    }
    if (key == '7') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 6;
    }
    if (key == '8') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 7;
    }
    if (key == '9') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 8;
    }
    if (key == '0') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 9;
    }
    if (key == 'z') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 10;
    }
    if (key == 'x') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 11;
    }
    if (key == 'c') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 12;
    }
    if (key == 'v') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 13;
    }
    if (key == 'b') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 14;
    }
    if (key == 'n') {
	    camera_state = BALL_LOCK_ON;
	    focused_ball = 15;
    }

    //glutPostRedisplay();
}

void reshape(int width, int height)
{
    glViewport(0, 0, 512, 512);
}

enum state {
    STRAIGHT_LINE_ROLL,
    SPIRAL_ROLL
};


int current_state = SPIRAL_ROLL;
bool first_round = true;

int ball_index = 0;
float tick = 0.0f;
float tick_speed = 0.01f;

void idle() {
	
	// CAMERA CONTROL
	if (camera_state == FREE_CAM) {
		tick_speed = 0.01f;
		ctm = mat_mult(
			xform_rot_mat('x', vert_pos),
			mat_mult(
				xform_rot_mat('y', horiz_pos),
				xform_scale_mat(scale_factor, scale_factor, scale_factor)
		      	)
		      );
	}
	else {
		tick_speed = 0.001f;

		ctm = mat_mult(
				xform_scale_mat(2.0f, 2.0f, 2.0f),
				mat_mult(
					xform_rot_mat('z', .05f),
					mat_mult(
						xform_rot_mat('x', .03f),
						mat_mult(
							xform_rot_mat('y', .25f * 3.14159),
							mat_inv(ball_current_mats[focused_ball])
							)
					      )
					)
				);
		//mat4 just_translation = xform_trans_mat(raw.w.x, raw.w.y, raw.w.z);
		//mat4 just_translation = xform_trans_mat(raw.w.x, raw.w.y, raw.w.z);
		/*
		mat4 just_translation = {
					{raw.x.w, 0.0f, 0.0f, 0.0f},
					{0.0f, raw.y.w, 0.0f, 0.0f},
					{0.0f, 0.0f, raw.z.w, 0.0f},
					{0.0f, 0.0f, 0.0f, 1.0f}};

					*/

	}



	// BALL CONTROL
	if (current_state == STRAIGHT_LINE_ROLL) {
		tick += 1.0f;
		ball_current_mats[ball_index] = mat_mult(xform_trans_mat(tick * ball_direction_vectors[ball_index].x, tick * ball_direction_vectors[ball_index].y, tick * ball_direction_vectors[ball_index].z), ball_start_mats[ball_index]);
		if (tick > 1.0f) {
			printf("DONE!\n");
			tick = 0.0f;
			if (ball_index != 15) {
				ball_index += 1;
			}
			else {
				current_state += 1;
				printf("Switching to spiral...\n");
				for (int i = 0; i < 16; ++i) {
					ball_start_mats[i] = mat_mult(xform_trans_mat(0.0f, -.1f, 0.0f), ball_current_mats[i]);

				}
			}
		};
	}

	if (current_state == SPIRAL_ROLL) {
		for (int i = 0; i < 16; ++i) {
			//ball_current_mats[i] = mat_mult(xform_rot_mat('y', (float)(i) * 0.001f), ball_current_mats[i]);
			/*
			ball_current_mats[i] = mat_mult(
					xform_trans_mat(0.0f, .1f, 0.0f),
					mat_mult(
						xform_rot_mat('y', (float)(i) * 0.001f), 
						mat_mult(
							xform_rot_mat('x', (float)(i) * 0.001f),
							mat_mult(
								xform_trans_mat(0.0f, -.1f, 0.0f),
								ball_current_mats[i]
							))));
							*/
			ball_current_mats[i] = mat_mult(
							xform_trans_mat(0.0f, .1f, 0.0f),
							mat_mult(
								xform_rot_mat('y', (float)(i+1) * tick * 0.1f),
								mat_mult(
									xform_rot_mat('z', 24.0 * tick * 0.1f), 
									ball_start_mats[i]

								)));

		}
		tick += tick_speed;
	}
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(100,100);
    glutCreateWindow("Pool Table");
    glewInit();
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMainLoop();

    return 0;
}

// --------------------------------------------------------------------------------

void generate_pool_balls() {

	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// ------------------------ PART 1: CREATE RAW VERTICES ---------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	// Approach:
	// STEP ONE - GENERATE RAW POINTS
	// Generate the top and bottom points
	// Generate a curved line of points
	// Rotate curved line of points around center to fill out sphere
	// STEP TWO - CREATE FACES
	// Create triangle fan on top and bottom
	// Stitch together grid for main part of sphere
	//
	
	int starting_vtx_index = vtx_index; // used for id creation later
	int sphere_index = 0;
	vec4 base_sphere[VERTS_PER_SPHERE];

	// main variables for generating sphere
	const float radius = 0.1f;
	const int horiz_div = 16;
	const int vert_div = 32;
	const float pi = 3.14159f;

	// get important points on sphere
	const vec4 center_point = { 0.0f, 0.0f, 0.0f, 1.0f };
	const vec4 temp = { 0.0f, radius, 0.0f, 0.0f };
	const vec4 top_point = vec_add(center_point, temp);
	const vec4 bot_point = vec_sub(center_point, temp);
	vec4 all_other_points[vert_div][horiz_div];

	// generate curved line of points
	for (int i = 0; i < horiz_div; ++i) {
		// get point from 0 to pi based on current round percent of number of horizontal divisions
		const float position = (float)(i + 1) / (float)(horiz_div + 1) * pi;

		// scale x and y results by the radis and move them to be relative to center point
		const vec4 point = {radius * sin(position) + center_point.x, radius * cos(position) + center_point.y, 0.0f, 1.0f };

		all_other_points[0][i] = point;

	}

	// get rotation based on number of vertical divisions
	const mat4 rot_mat = xform_rot_mat('y', 1.0f/vert_div * (2.0f*pi));

	// rotate points to create points 
	for (int i = 1; i < vert_div; ++i) {
		for (int j = 0; j < horiz_div; ++j) {
			all_other_points[i][j] = mat_vec_mult(rot_mat, all_other_points[i-1][j]);
		}
	}


	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// ----------------------------- PART 2: CREATE FACES -----------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	// stitch togehter main body of sphere
	for (int i = 0; i < vert_div; ++i) {
		for (int j = 0; j < horiz_div-1; ++j) {
			base_sphere[sphere_index] = all_other_points[i][j];
			sphere_index++;
			base_sphere[sphere_index] = all_other_points[i][(j+1)%horiz_div]; sphere_index++; base_sphere[sphere_index] = all_other_points[(i+1)%vert_div][(j+1)%horiz_div];
			sphere_index++;

			base_sphere[sphere_index] = all_other_points[i][j];
			sphere_index++;
			base_sphere[sphere_index] = all_other_points[(i+1)%vert_div][(j+1)%horiz_div];
			sphere_index++;
			base_sphere[sphere_index] = all_other_points[(i+1)%vert_div][j];
			sphere_index++;
		}
	}
	printf("NUM OF VERTS W/O CAPS: %i\n", sphere_index);
	

	// generate caps
	for (int i = 0; i < vert_div; ++i) { // top
		base_sphere[sphere_index] = top_point;
		sphere_index++;
		base_sphere[sphere_index] = all_other_points[i][0];
		sphere_index++;
		base_sphere[sphere_index] = all_other_points[(i+1)%vert_div][0];
		sphere_index++;
	}

	for (int i = 0; i < vert_div; ++i) { // bottom
		base_sphere[sphere_index] = bot_point;
		sphere_index++;
		base_sphere[sphere_index] = all_other_points[(i+1)%vert_div][horiz_div-1];
		sphere_index++;
		base_sphere[sphere_index] = all_other_points[i][horiz_div-1];
		sphere_index++;
	}

	printf("TOTAL NUM OF VERTS: %i\n", sphere_index);

	// rotate so that correct side of sphere is facing up
	mat4 xform = xform_rot_mat('x', 0.5f * pi);
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) { base_sphere[i] = mat_vec_mult(xform, base_sphere[i]); }
	
	// rotate so numbers are oriented properly
	xform = xform_rot_mat('y', pi);
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) { base_sphere[i] = mat_vec_mult(xform, base_sphere[i]); }

	// move base sphere up to proper y position
	//xform = xform_trans_mat(0.0f, 0.1f, 0.0f);
	//for (int i = 0; i < VERTS_PER_SPHERE; ++i) { base_sphere[i] = mat_vec_mult(xform, base_sphere[i]); }


	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// ----------------------------- PART 3: GENERATE UVS -----------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	// array that hold the coordinates for the base sphere
	GLfloat reference_uvs[VERTS_PER_SPHERE][2];

	// used to improve code readability

	// vec4 all_other_points[vert_div][horiz_div];
	
	/*
	for (int i = 0; i < TOTAL_VERTICES; ++i) {
		tex_coords[i][0] = 0.28f;
		tex_coords[i][1] = 0.28f;
		//printf("X = %f, Y = %f\n", tex_coords[i][0], tex_coords[i][1]);

	}
	*/
	
	int uv_index = starting_vtx_index;
	// generate main uvs 
	for (int i = 0; i < vert_div/2; ++i) {
		for (int j = 0; j < horiz_div-1; ++j) {


			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;


			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

		}
	}
	for (int i = 0; i < vert_div/2; ++i) {
		for (int j = 0; j < horiz_div-1; ++j) {

			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;


			reference_uvs[uv_index][0] = (float)i/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)(j+1)/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

			reference_uvs[uv_index][0] = (float)(i+1)/((float)vert_div/2) * 0.25f * 0.92f + 0.01f;
			reference_uvs[uv_index][1] = (float)j/(float)horiz_div * 0.25f * 0.92f + 0.01f;
			uv_index++;

		}
	}
	// the caps do not ever contain anything besides solid color so they can just be jammed onto the uvs
	for (int i = 0; i < 100; ++i) {
		reference_uvs[uv_index][0] = 0.02f;
		reference_uvs[uv_index][1] = 0.02f;
		uv_index++;
	}
	printf("NUM OF VERTS W/O CAPS: %i\n", sphere_index);
	



	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// --------------------------- PART 4: GENERATE NORMALS ---------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------

	vec4 ref_normals[TOTAL_VERTICES];

	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		ref_normals[i] = vec_norm(vec_sub(base_sphere[i], center_point));
		ref_normals[i].w = 1.0f;
		//vec_print(ref_normals[i]);
	}
	

	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------
	// ------------------------- PART 5: CREATE ALL 16 BALLS --------------------------
	// --------------------------------------------------------------------------------
	// --------------------------------------------------------------------------------


	xform = identity;
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]);
		tex_coords[vtx_index][0] = reference_uvs[i][0];
		tex_coords[vtx_index][1] = reference_uvs[i][1];
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 1;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .25f;
		tex_coords[vtx_index][1] = reference_uvs[i][1];
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 2;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .50f;
		tex_coords[vtx_index][1] = reference_uvs[i][1];
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 3;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .75f;
		tex_coords[vtx_index][1] = reference_uvs[i][1];
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 4;
		vtx_index++;
	}

	


	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]);
		tex_coords[vtx_index][0] = reference_uvs[i][0];
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .25f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 5;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .25f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .25f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 6;
		vtx_index++;

	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) { 
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .50f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .25f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 7;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .75f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .25f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 8;
		vtx_index++;
	}


	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]);
		tex_coords[vtx_index][0] = reference_uvs[i][0];
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .50f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 9;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .25f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .50f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 10;
		vtx_index++;

	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) { 
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .50f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .50f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 11;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .75f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .50f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 12;
		vtx_index++;
	}

	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]);
		tex_coords[vtx_index][0] = reference_uvs[i][0];
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .75f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 13;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .25f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .75f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 14;
		vtx_index++;

	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) { 
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .50f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .75f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 15;
		vtx_index++;
	}
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		tex_coords[vtx_index][0] = reference_uvs[i][0] + .75f;
		tex_coords[vtx_index][1] = reference_uvs[i][1] + .75f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 16;
		vtx_index++;
	}
	//LIGHT
	xform = mat_mult(xform_trans_mat(0.0f, 0.0f, 0.0f), xform_scale_mat(0.5f, 0.5f, 0.5f));
	for (int i = 0; i < VERTS_PER_SPHERE; ++i) {
		vertices[vtx_index] = mat_vec_mult(xform, base_sphere[i]); 
		//tex_coords[vtx_index][0] = reference_uvs[i][0] + .75f;
		//tex_coords[vtx_index][1] = reference_uvs[i][1] + .75f;
		colors[vtx_index].x = 1.0f;
		colors[vtx_index].y = 1.0f;
		colors[vtx_index].z = 1.0f;
		colors[vtx_index].w = 1.0f;
		normals[vtx_index] = ref_normals[i];
		id[vtx_index] = 16;
		vtx_index++;
	}

	// TABLE
	vec4 temp_vtx1 = { -5.0f, 0.0f, -5.0f, 1.0f };
	vec4 temp_vtx2 = { -5.0f, 0.0f, 5.0f, 1.0f };
	vec4 temp_vtx3 = { 5.0f, 0.0f, 5.0f, 1.0f };

	vertices[vtx_index] = temp_vtx1;
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
	vertices[vtx_index] = temp_vtx2;
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
	vertices[vtx_index] = temp_vtx3;
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
	mat4 rotator = xform_rot_mat('y', pi);
	vertices[vtx_index] = mat_vec_mult(rotator, temp_vtx1);
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
	vertices[vtx_index] = mat_vec_mult(rotator, temp_vtx2);
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
	vertices[vtx_index] = mat_vec_mult(rotator, temp_vtx3);
	colors[vtx_index].y = 1.0f;
	normals[vtx_index].y = 1.0f;
	vtx_index++;
}

void initialize_matricies() {
	// STARTING MATRICIES
	ball_start_mats[0] = xform_trans_mat(0.0f, 0.0f, -3.0f);
	ball_start_mats[1] = xform_trans_mat(0.0f, 0.0f, -2.8f);
	ball_start_mats[2] = xform_trans_mat(0.0f, 0.0f, -2.6f);
	ball_start_mats[3] = xform_trans_mat(0.0f, 0.0f, -2.4f);

	ball_start_mats[4] = xform_trans_mat(0.0f, 0.0f, -2.2f);
	ball_start_mats[5] = xform_trans_mat(0.0f, 0.0f, -2.0f);
	ball_start_mats[6] = xform_trans_mat(0.0f, 0.0f, -1.8f);
	ball_start_mats[7] = xform_trans_mat(0.0f, 0.0f, -1.6f);

	ball_start_mats[8] = xform_trans_mat(0.0f, 0.0f, -1.4f);
	ball_start_mats[9] = xform_trans_mat(0.0f, 0.0f, -1.2f);
	ball_start_mats[10] = xform_trans_mat(0.0f, 0.0f, -1.0f);
	ball_start_mats[11] = xform_trans_mat(0.0f, 0.0f, -0.8f);

	ball_start_mats[12] = xform_trans_mat(0.0f, 0.0f, -0.6f);
	ball_start_mats[13] = xform_trans_mat(0.0f, 0.0f, -0.4f);
	ball_start_mats[14] = xform_trans_mat(0.0f, 0.0f, -0.2f);
	ball_start_mats[15] = xform_trans_mat(0.0f, 0.0f, 0.0f);


	for (int i = 0; i < 16; ++i) {
		ball_current_mats[i] = ball_start_mats[i];
	}

	// STARTING POSITIONS
	for (int i = 0; i < 16; ++i) {
		ball_start_pos[i] = mat_vec_mult(ball_start_mats[i], zero_vec);
		vec_print(ball_start_pos[i]);
	}
	printf("\n");

	// ROLLING IN A STRAIGHT LINE ENDING POSITIONS
	for (int i = 0; i < 16; i++) {
		vec4 temp = {0.0f, 0.1f, -(15.0f - (float)i) * 0.2f, 1.0f};
		ball_rolling_end_positions[i] = temp;
	}
	for (int i = 0; i < 16; ++i) {
		vec_print(ball_rolling_end_positions[i]);
	}
	printf("\n");

	// VECTOR FROM START TO FINISH
	for (int i = 0; i < 16; ++i) {
		ball_direction_vectors[i] = vec_sub(ball_rolling_end_positions[i], ball_start_pos[i]);
		ball_direction_vectors[i].w = 1.0f;
		vec_print(ball_direction_vectors[i]);
	}

}
