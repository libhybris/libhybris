/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>

const char vertex_src [] =
"                                        \
   attribute vec4        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
                                         \
   void main()                           \
   {                                     \
      gl_Position = position + offset;   \
      pos = position.xy;                 \
   }                                     \
";


const char fragment_src [] =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
   }                                                   \
";

GLuint load_shader(const char *shader_source, GLenum type)
{
	GLuint  shader = glCreateShader(type);

	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);

	return shader;
}


GLfloat norm_x    =  0.0;
GLfloat norm_y    =  0.0;
GLfloat offset_x  =  0.0;
GLfloat offset_y  =  0.0;
GLfloat p1_pos_x  =  0.0;
GLfloat p1_pos_y  =  0.0;

GLint phase_loc;
GLint offset_loc;
GLint position_loc;

const float vertexArray[] = {
	0.0,  1.0,  0.0,
	-1.,  0.0,  0.0,
	0.0, -1.0,  0.0,
	1.,  0.0,  0.0,
	0.0,  1.,  0.0
};


int main(int argc, char **argv)
{
	EGLDisplay display;
	EGLConfig ecfg;
	EGLint num_config;
	EGLint attr[] = {       // some attributes to set up our egl-interface
		EGL_BUFFER_SIZE, 32,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLSurface surface;
	EGLint ctxattr[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLContext context;

	EGLBoolean rv;

	display = eglGetDisplay(NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(display != EGL_NO_DISPLAY);

	rv = eglInitialize(display, 0, 0);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, (EGLNativeWindowType)NULL, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(surface != EGL_NO_SURFACE);

	context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
	assert(eglGetError() == EGL_SUCCESS);
	assert(context != EGL_NO_CONTEXT);

	assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);

	const char *version = (const char *)glGetString(GL_VERSION);
	assert(version);
	printf("%s\n",version);


	GLuint vertexShader   = load_shader ( vertex_src , GL_VERTEX_SHADER  );     // load vertex shader
	GLuint fragmentShader = load_shader ( fragment_src , GL_FRAGMENT_SHADER );  // load fragment shader

	GLuint shaderProgram  = glCreateProgram ();                 // create program object
	glAttachShader ( shaderProgram, vertexShader );             // and attach both...
	glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it

	glLinkProgram ( shaderProgram );    // link the program
	glUseProgram  ( shaderProgram );    // and select it for usage

	//// now get the locations (kind of handle) of the shaders variables
	position_loc  = glGetAttribLocation  ( shaderProgram , "position" );
	phase_loc     = glGetUniformLocation ( shaderProgram , "phase"    );
	offset_loc    = glGetUniformLocation ( shaderProgram , "offset"   );
	if ( position_loc < 0  ||  phase_loc < 0  ||  offset_loc < 0 ) {
		return 1;
	}

	//glViewport ( 0 , 0 , 800, 600); // commented out so it uses the initial window dimensions
	glClearColor ( 1. , 1. , 1. , 1.);    // background color
	float phase = 0;
	int i;
	for (i=0; i<3*60; ++i) {
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1f ( phase_loc , phase );  // write the value of phase to the shaders phase
		phase  =  fmodf ( phase + 0.5f , 2.f * 3.141f );    // and update the local variable

		glUniform4f ( offset_loc  ,  offset_x , offset_y , 0.0 , 0.0 );

		glVertexAttribPointer ( position_loc, 3, GL_FLOAT, GL_FALSE, 0, vertexArray );
		glEnableVertexAttribArray ( position_loc );
		glDrawArrays ( GL_TRIANGLE_STRIP, 0, 5 );

		eglSwapBuffers ( (EGLDisplay) display, surface );  // get the rendered buffer to the screen
	}

	printf("stop\n");

#if 0
(*egldestroycontext)((EGLDisplay) display, context);
    printf("destroyed context\n");

    (*egldestroysurface)((EGLDisplay) display, surface);
    printf("destroyed surface\n");
    (*eglterminate)((EGLDisplay) display);
    printf("terminated\n");
    android_dlclose(baz);
#endif
}

// vim:ts=4:sw=4:noexpandtab
