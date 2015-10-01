#include "gles.h"
#include "context.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#ifdef RASPBERRYPI
#include <bcm_host.h>
#endif

#define	SHOW_ERROR	gles_show_error();

/*static const char* vertex_shader_prg =
    "uniform mat4 u_vp_matrix;                              \n"
    "attribute vec4 a_position;                             \n"
    "attribute vec2 a_texcoord;                             \n"
    "varying mediump vec2 v_texcoord;                       \n"
    "void main()                                            \n"
    "{                                                      \n"
    "   v_texcoord = a_texcoord;                            \n"
    "   gl_Position = u_vp_matrix * a_position;             \n"
    "}                                                      \n";

static const char* fragment_shader_noeffect_16bit =
    "varying mediump vec2 v_texcoord;                       \n"
    "uniform sampler2D u_texture;                           \n"
    "void main()                                            \n"
    "{                                                      \n"
	"	gl_FragColor = texture2D(u_texture, v_texcoord);	\n"
    "}                                                      \n";
*/

static const char* vertex_shader_custom =
	"uniform mat4 MVPMatrix;"
	"uniform mediump vec2 OutputSize;"
	"uniform mediump vec2 TextureSize;"
	"uniform mediump vec2 InputSize;"

	"attribute vec4 VertexCoord;"
	"attribute vec4 TexCoord;"
	"varying vec4 TEX0;"
	"varying vec4 TEX2;"
	"varying     vec2 _omega;"

	"struct sine_coord {"
	"    vec2 _omega;"
	"};"

	"vec4 _oPosition1;"
	"vec4 _r0006;"
	 
	"void main()"
	"{"
	"    vec2 _oTex;"
	"    sine_coord _coords;"
	"    _r0006 = VertexCoord.x*MVPMatrix[0];"
	"    _r0006 = _r0006 + VertexCoord.y*MVPMatrix[1];"
	"    _r0006 = _r0006 + VertexCoord.z*MVPMatrix[2];"
	"    _r0006 = _r0006 + VertexCoord.w*MVPMatrix[3];"
	"    _oPosition1 = _r0006;"
	"    _oTex = TexCoord.xy;"
	"    _coords._omega = vec2((3.14150000E+00*OutputSize.x*TextureSize.x)/InputSize.x, 6.28299999E+00*TextureSize.y);"
	"    gl_Position = _r0006;"
	"    TEX0.xy = TexCoord.xy;"
	"    TEX2.xy = _coords._omega;"
	"}";

static const char* fragment_shader_custom_scanlines_16bit =
	"precision mediump float;"

	"uniform mediump vec2 OutputSize;"
	"uniform mediump vec2 TextureSize;"
	"uniform mediump vec2 InputSize;"
	"uniform sampler2D Texture;"

	"varying     vec2 _omega;"
	"varying vec4 TEX2;"
	"varying vec4 TEX0;"

	"struct sine_coord {"
	"    vec2 _omega;"
	"};"
	"vec4 _ret_0;"
	"float _TMP2;"
	"vec2 _TMP1;"
	"float _TMP4;"
	"float _TMP3;"
	"vec4 _TMP0;"
	"vec2 _x0009;"
	"vec2 _a0015;"

	"void main()"
	"{"
	"    vec3 _scanline;"
	"    _TMP0 = texture2D(Texture, TEX0.xy);"
	"    _x0009 = TEX0.xy*TEX2.xy;"
	"    _TMP3 = sin(_x0009.x);"
	"    _TMP4 = sin(_x0009.y);"
	"    _TMP1 = vec2(_TMP3, _TMP4);"
	"    _a0015 = vec2( 5.00000007E-02, 1.50000006E-01)*_TMP1;"
	"    _TMP2 = dot(_a0015, vec2( 1.00000000E+00, 1.00000000E+00));"
	"    _scanline = _TMP0.xyz*(9.49999988E-01 + _TMP2);"
	"    _ret_0 = vec4(_scanline.x, _scanline.y, _scanline.z, 1.00000000E+00);"
	"    gl_FragColor = _ret_0;"
	"    return;"
	"}"
;

static const GLfloat vertices[] =
{
	-0.5f, -0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	-0.5f, +0.5f, 0.0f,
};

//Values defined in gles2_create()
static GLfloat uvs[8];

static const GLushort indices[] =
{
	0, 1, 2,
	0, 2, 3,
};

typedef struct {
	unsigned display_width;
	unsigned display_height;

	unsigned texture_width;
	unsigned texture_height;

	float ratio_x;
	float ratio_y;
	unsigned output_width;
	unsigned output_height;

	void *pixels;
} dispvars_t;

dispvars_t dispvars;
dispvars_t* _dispvars = &dispvars;

static const unsigned kVertexCount = 4;
static const unsigned kIndexCount = 6;

void gles_show_error()
{
	GLenum error = GL_NO_ERROR;
	error = glGetError();
	if (GL_NO_ERROR != error) {
		printf("GL Error %x encountered!\n", error);
		exit(0);
	}
}

static GLuint CreateShader(GLenum type, const char *shader_src)
{
	GLuint shader = glCreateShader(type);
	if(!shader)
		return 0;

	// Load and compile the shader source
	glShaderSource(shader, 1, &shader_src, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(sizeof(char) * info_len);
			glGetShaderInfoLog(shader, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			printf("Error compiling shader:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

// Function to load both vertex and fragment shaders, and create the program
static GLuint CreateProgram(const char *vertex_shader_src, const char *fragment_shader_src)
{
	GLuint vertex_shader = CreateShader(GL_VERTEX_SHADER, vertex_shader_src);
	if(!vertex_shader)
		return 0;
	GLuint fragment_shader = CreateShader(GL_FRAGMENT_SHADER, fragment_shader_src);
	if(!fragment_shader)
	{
		glDeleteShader(vertex_shader);
		return 0;
	}

	GLuint program_object = glCreateProgram();
	if(!program_object)
		return 0;
	glAttachShader(program_object, vertex_shader);
	glAttachShader(program_object, fragment_shader);

	// Link the program
	glLinkProgram(program_object);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(program_object, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint info_len = 0;
		glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_len);
		if(info_len > 1)
		{
			char* info_log = (char *)malloc(info_len);
			glGetProgramInfoLog(program_object, info_len, NULL, info_log);
			// TODO(dspringer): We could really use a logging API.
			printf("Error linking program:\n%s\n", info_log);
			free(info_log);
		}
		glDeleteProgram(program_object);
		return 0;
	}
	// Delete these here because they are attached to the program object.
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program_object;
}

static void SetOrtho(float m[4][4], float left, float right, float bottom, float top, float near, float far, float scale_x, float scale_y)
{
	memset(m, 0, 4*4*sizeof(float));
	m[0][0] = 2.0f/(right - left)*scale_x;
	m[1][1] = 2.0f/(top - bottom)*scale_y;
	m[2][2] = -2.0f/(far - near);
	m[3][0] = -(right + left)/(right - left);
	m[3][1] = -(top + bottom)/(top - bottom);
	m[3][2] = -(far + near)/(far - near);
	m[3][3] = 1;
}

typedef struct ShaderInfo
{
   GLuint program;
   GLint u_vp_matrix;
   GLint u_texture;
   GLint a_position; // vertex_coord;
   GLint a_texcoord; //	tex_coord;
   GLint a_color;    // color
   
   GLint lut_tex_coord;

   GLint input_size;
   GLint output_size;
   GLint texture_size;
} ShaderInfo;

static ShaderInfo shader;
static GLuint buffers[3];
static GLuint texture;

static float proj[4][4];

/*void gles2_init_shaders_orig () {
	memset(&shader, 0, sizeof(ShaderInfo));

	// Load default shaders
	shader.program = CreateProgram(vertex_shader_prg, fragment_shader_noeffect_16bit);

	if(shader.program)
	{
		shader.a_position	= glGetAttribLocation(shader.program, "a_position");
		shader.a_texcoord	= glGetAttribLocation(shader.program, "a_texcoord");
		shader.u_vp_matrix	= glGetUniformLocation(shader.program, "u_vp_matrix");
		shader.u_texture	= glGetUniformLocation(shader.program, "u_texture");
	}	

	if(!shader.program) 
		exit(0);
	
	glUseProgram(shader.program); SHOW_ERROR
}*/

void gles2_init_shaders () {
	memset(&shader, 0, sizeof(ShaderInfo));

	// Load custom shaders
   	float input_size[2], output_size[2], texture_size[2];
	
	shader.program = CreateProgram(vertex_shader_custom, fragment_shader_custom_scanlines_16bit);

	if(shader.program)
	{
		shader.u_vp_matrix   = glGetUniformLocation(shader.program, "MVPMatrix");
  	 	shader.a_texcoord    = glGetAttribLocation(shader.program, "TexCoord");
		shader.a_position    = glGetAttribLocation(shader.program, "VertexCoord");
		
		shader.input_size    = glGetUniformLocation(shader.program, "InputSize");
		shader.output_size   = glGetUniformLocation(shader.program, "OutputSize");
		shader.texture_size  = glGetUniformLocation(shader.program, "TextureSize");
	}

	input_size [0]  = _dispvars->texture_width;
   	input_size [1]  = _dispvars->texture_height;
	output_size[0]  = _dispvars->output_width;
	output_size[1]  = _dispvars->output_height;
	texture_size[0] = _dispvars->texture_width;
	texture_size[1] = _dispvars->texture_height;

	if(!shader.program) 
		exit(0);
	
	glUseProgram(shader.program); SHOW_ERROR

	glUniform2fv(shader.input_size, 1, input_size);
	glUniform2fv(shader.output_size, 1, output_size);
	glUniform2fv(shader.texture_size, 1, texture_size);
}

void gles2_init_texture () {
	//create bitmap texture
	glGenTextures(1, &texture);	SHOW_ERROR

	// For 32 bpp textures
   	//glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	
	glBindTexture(GL_TEXTURE_2D, texture); SHOW_ERROR

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _dispvars->texture_width, _dispvars->texture_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL); SHOW_ERROR
	
	// For 32 bpp textures
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _dispvars->texture_width, _dispvars->texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); SHOW_ERROR
	//	

	glActiveTexture(GL_TEXTURE0); SHOW_ERROR
	glBindTexture(GL_TEXTURE_2D, texture); SHOW_ERROR
}

void gles2_init_geometry () {
	// Setup texture coordinates
	float min_u=0;
	float max_u=1.0f;
	float min_v=0;
	float max_v=1.0f;

	uvs[0] = min_u;
	uvs[1] = min_v;
	uvs[2] = max_u;
	uvs[3] = min_v;
	uvs[4] = max_u;
	uvs[5] = max_v;
	uvs[6] = min_u;
	uvs[7] = max_v;
	// 

	glGenBuffers(3, buffers); SHOW_ERROR
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); SHOW_ERROR
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 3, vertices, GL_STATIC_DRAW); SHOW_ERROR
	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]); SHOW_ERROR
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 2, uvs, GL_STATIC_DRAW); SHOW_ERROR
	glBindBuffer(GL_ARRAY_BUFFER, 0); SHOW_ERROR
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]); SHOW_ERROR
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexCount * sizeof(GL_UNSIGNED_SHORT), indices, GL_STATIC_DRAW); SHOW_ERROR
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); SHOW_ERROR

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); SHOW_ERROR
	
	glDisable(GL_DEPTH_TEST); SHOW_ERROR
	glDisable(GL_DITHER); SHOW_ERROR
	glDisable(GL_BLEND); SHOW_ERROR

       	SetOrtho(proj, -0.5f, +0.5f, +0.5f, -0.5f, -1.0f, 1.0f, _dispvars->ratio_x, _dispvars->ratio_y);

	// We activate the position and texture coordinate attributes for the vertices
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); SHOW_ERROR
	glVertexAttribPointer(shader.a_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), NULL); SHOW_ERROR
	glEnableVertexAttribArray(shader.a_position); SHOW_ERROR

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]); SHOW_ERROR
	glVertexAttribPointer(shader.a_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL); SHOW_ERROR
	glEnableVertexAttribArray(shader.a_texcoord); SHOW_ERROR

	// Viewport is configured to the size of the quad on whic we draw the texture.
	glViewport(0, 0, _dispvars->display_width, _dispvars->display_height); SHOW_ERROR

	// We upload the projection matrix
	glUniformMatrix4fv(shader.u_vp_matrix, 1, GL_FALSE, &proj[0][0]); SHOW_ERROR

	// We leave the element array buffer binded so it's binded when we arrive to gles2_draw() on each frame.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]); SHOW_ERROR
}

void gles2_init_dimensions (unsigned texture_width, unsigned texture_height, bool maintain_aspect_ratio) {
	// eglInfo.width and eglInfo.height must be provided by the egl init function, 
	// where native display dimensions are detected (or forcer if we use X11).
	_dispvars->texture_width = (float)texture_width;
	_dispvars->texture_height = (float)texture_height;

	// MAC: In case we apply aspect ratio correction, we have to pass the aspect-corrected texture dimensions
	// to the scanlines shader so it compensates internally for the correction and scanlines look right.  
	_dispvars->ratio_x = 1.0f;
	_dispvars->ratio_y = 1.0f;
	//_dispvars->output_width = _dispvars->display_width;
	//_dispvars->output_height = _dispvars->display_height;
	_dispvars->output_width = eglInfo.width;
	_dispvars->output_height = eglInfo.height;
	_dispvars->display_width = eglInfo.width;
	_dispvars->display_height = eglInfo.height;
 
	if (maintain_aspect_ratio) {
		float display_ratio = (float)_dispvars->display_height/(float)_dispvars->display_width;
		float texture_ratio = (float)_dispvars->texture_height/(float)_dispvars->texture_width;

		if(texture_ratio > display_ratio) 
			_dispvars->ratio_x = display_ratio/texture_ratio;
		else
			_dispvars->ratio_y = texture_ratio/display_ratio;
		
		_dispvars->output_width  = _dispvars->display_width  * _dispvars->ratio_x;
		_dispvars->output_height = _dispvars->display_height * _dispvars->ratio_y;
	}
}

void gles2_init(int texture_width, int texture_height, int depth, bool maintain_aspect_ratio)
{
	gles2_init_dimensions(texture_width, texture_height, maintain_aspect_ratio);
	gles2_init_shaders();
	gles2_init_texture();
	gles2_init_geometry();
}

void gles2_destroy()
{
	if(!shader.program) return;
	if(shader.program) glDeleteProgram(shader.program);
	glDeleteBuffers(3, buffers); SHOW_ERROR
	glDeleteTextures(1, &texture); SHOW_ERROR
}

void gles2_draw(void *pixels)
{
	//glClear(GL_COLOR_BUFFER_BIT); SHOW_ERROR

	// Update texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _dispvars->texture_width, _dispvars->texture_height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (unsigned short*) pixels); SHOW_ERROR
	
	// For 32 bpp textures
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _dispvars->texture_width, _dispvars->texture_height, GL_RGB, GL_UNSIGNED_BYTE, (unsigned short*) _dispvars->pixel); SHOW_ERROR
	
	// Draw geometry
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]); SHOW_ERROR
	glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0); SHOW_ERROR

	//glBindBuffer(GL_ARRAY_BUFFER, 0); SHOW_ERROR
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); SHOW_ERROR
}

/*void pi_deinit(void)
{
	// Free GLES2 stuff
	gles2_destroy();

	// Free EGL stuff
	eglMakeCurrent( _dispvars->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroySurface( _dispvars->egl_display, _dispvars->egl_surface );
	eglDestroyContext( _dispvars->egl_display, _dispvars->egl_context );
	eglTerminate( _dispvars->egl_display );

	// Free DISPMANX stuff
	_dispvars->dispman_update = vc_dispmanx_update_start( 0 );
	vc_dispmanx_element_remove( _dispvars->dispman_update, _dispvars->dispman_element );
	vc_dispmanx_element_remove( _dispvars->dispman_update, _dispvars->dispman_element_bg );
	vc_dispmanx_update_submit_sync( _dispvars->dispman_update );
	vc_dispmanx_display_close( _dispvars->dispman_display );

	bcm_host_deinit();
}*/
