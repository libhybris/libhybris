#define MESA_EGL_NO_X11_HEADERS
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

GLuint
load_shader (
   const char  *shader_source,
   GLenum       type
)
{
   GLuint  shader = glCreateShader( type );

   glShaderSource  ( shader , 1 , &shader_source , NULL );
   glCompileShader ( shader );

   return shader;
}


GLfloat
   norm_x    =  0.0,
   norm_y    =  0.0,
   offset_x  =  0.0,
   offset_y  =  0.0,
   p1_pos_x  =  0.0,
   p1_pos_y  =  0.0;

GLint
   phase_loc,
   offset_loc,
   position_loc;



const float vertexArray[] = {
   0.0,  0.5,  0.0,
  -0.5,  0.0,  0.0,
   0.0, -0.5,  0.0,
   0.5,  0.0,  0.0,
   0.0,  0.5,  0.0 
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
	display = eglGetDisplay(NULL);

	eglInitialize(display, 0, 0);
        eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);
	surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, (EGLNativeWindowType)NULL, NULL);
	assert(surface != EGL_NO_SURFACE);
	context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
	assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
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

        
      glViewport ( 0 , 0 , 800, 600);
      glClearColor ( 0.08 , 0.06 , 0.07 , 1.);    // background color
    float phase = 0;
   while (1) {
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
