#define MESA_EGL_NO_X11_HEADERS
//#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "fbdev_window.h"
#include "offscreen_window.h"
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR=0;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR=0;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES=0;

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        fprintf(stderr, "after %s() glError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] = 
    "attribute vec4 vPosition;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  yuvTexCoords = vPosition.xy + vec2(0.5, 0.5);\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char gFragmentShader[] = 
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES yuvTexSampler;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            } else {
                fprintf(stderr, "Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    fprintf(stderr, "Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLint gvPositionHandle;
GLint gYuvTexSamplerHandle;
const GLfloat gTriangleVertices[] = {
    -0.5f, 0.5f,
    -0.5f, -0.5f,
    0.5f, -0.5f,
    0.5f, 0.5f,
};


class EGLClient {
public:
    EGLClient() {
        EGLConfig ecfg;
        EGLint num_config;
        EGLint attr[] = {       // some attributes to set up our egl-interface
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };
        EGLint ctxattr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            printf("ERROR: Could not get default display\n");
            return;
        }

        printf("INFO: Successfully retrieved default display!\n");

        eglInitialize(display, 0, 0);
        eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);

        printf("INFO: Initialized display with default configuration\n");

        window = new OffscreenNativeWindow(720, 1280);
        printf("INFO: Created native window %p\n", window);
        printf("creating window surface...\n");
        surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, *window, NULL);
        assert(surface != EGL_NO_SURFACE);
        printf("INFO: Created our main window surface %p\n", surface);
        context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
        printf("INFO: Created context for display\n");
        frame=0;
    };
    void render() {
            assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
            printf("INFO: Made context and surface current for display\n");
            glViewport ( 0 , 0 , 1280, 720);
            printf("client frame %i\n", frame++);
            glClearColor ( 1.00 , (frame & 1) * 1.0f , ((float)(frame % 255))/255.0f, 1.);    // background color
            glClear(GL_COLOR_BUFFER_BIT);
            eglSwapBuffers(display, surface);
            printf("client swapped\n");
    }
    int frame;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    OffscreenNativeWindow* window;
};

class EGLCompositor {
public:
    EGLCompositor() {
        EGLConfig ecfg;
        EGLint num_config;
        EGLint attr[] = {       // some attributes to set up our egl-interface
            EGL_BUFFER_SIZE, 32,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };
        EGLint ctxattr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            printf("ERROR: Could not get default display\n");
            return;
        }

        printf("INFO: Successfully retrieved default display!\n");

        eglInitialize(display, 0, 0);
        eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);

        printf("INFO: Initialized display with default configuration\n");

        window = new FbDevNativeWindow();
        printf("INFO: Created native window %p\n", window);
        printf("creating window surface...\n");
        surface = eglCreateWindowSurface((EGLDisplay) display, ecfg, *window, NULL);
        assert(surface != EGL_NO_SURFACE);
        printf("INFO: Created our main window surface %p\n", surface);
        context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT, ctxattr);
        assert(surface != EGL_NO_CONTEXT);
        printf("INFO: Created context for display\n");
        assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
        printf("INFO: Made context and surface current for display\n");
        frame=0;
        glGenTextures(1, &texture);
        
        gProgram = createProgram(gVertexShader, gFragmentShader);
        if (!gProgram) {
            printf("no program\n");
            abort();
        }
        gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
        checkGlError("glGetAttribLocation");
        fprintf(stderr, "glGetAttribLocation(\"vPosition\") = %d\n",
                gvPositionHandle);
        gYuvTexSamplerHandle = glGetUniformLocation(gProgram, "yuvTexSampler");
        checkGlError("glGetUniformLocation");
        fprintf(stderr, "glGetUniformLocation(\"yuvTexSampler\") = %d\n",
                gYuvTexSamplerHandle);

        glViewport(0, 0, 1280, 720);
        checkGlError("glViewport");

    };
    void render(OffscreenNativeWindow* window) {
            assert(eglMakeCurrent((EGLDisplay) display, surface, surface, context) == EGL_TRUE);
            printf("INFO: Made context and surface current for display\n");


            EGLClientBuffer cbuf = (EGLClientBuffer) window->getFrontBuffer();
            EGLint attrs[] = {
                EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
                EGL_NONE,
            };
            EGLImageKHR image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, cbuf, attrs);
            if (image == EGL_NO_IMAGE_KHR) {
                EGLint error = eglGetError();
                printf("error creating EGLImage: %#x", error);
            }
            printf("got egl image %p\n", image);

            glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
            int err;
            if(err = glGetError()) printf("%i gl error %x\n", __LINE__, err);
            glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);
            if(err = glGetError()) printf("%i gl error %x\n", __LINE__, err);
            glEnable(GL_TEXTURE_EXTERNAL_OES);
            if(err = glGetError()) printf("%i gl error %x\n", __LINE__, err);



            glViewport ( 0 , 0 , 1280, 720);
            printf("compositor frame %i\n", frame++);
            float c = (frame % 64) / 64.0f;
            glClearColor ( c , c , c, 1.);    // background color
    checkGlError("glClearColor");


    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");

    glUniform1i(gYuvTexSamplerHandle, 0);
    checkGlError("glUniform1i");
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    checkGlError("glBindTexture");

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArrays");



            eglSwapBuffers(display, surface);
            eglDestroyImageKHR(display, image);
            printf("compositor swapped\n");
    }
    int frame;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    FbDevNativeWindow* window;
    GLuint texture;
    EGLImageKHR image;
};

int main(int argc, char **argv)
{
    EGLCompositor compositor;
    eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
    eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");

    
    EGLClient client;
    while(1) {
        client.render();
        compositor.render(client.window);
    }
}
