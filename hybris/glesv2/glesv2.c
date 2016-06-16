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

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>

#include <egl/ws.h>
#include <hybris/common/binding.h>
#include <hybris/common/floating_point_abi.h>

static void *_libglesv2 = NULL;

/* Only functions with floating point argument need a wrapper to change the call convention correctly */

#define GLES2_LOAD(sym)  { *(&_ ## sym) = (void *) android_dlsym(_libglesv2, #sym);  } 

/*
This generates a function that when first called overwrites it's plt entry with new address. Subsequent calls jump directly at the target function in the android library. This means effectively 0 call overhead after the first call.
*/

#define GLES2_IDLOAD(sym) \
 __asm__ (".type " #sym ", %gnu_indirect_function"); \
typeof(sym) * sym ## _dispatch (void) __asm__ (#sym);\
typeof(sym) * sym ## _dispatch (void) \
{ \
	return (void *) android_dlsym(_libglesv2, #sym); \
} 

static void  __attribute__((constructor)) _init_androidglesv2()  {
	_libglesv2 = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("LIBGLESV2") : "libGLESv2.so", RTLD_NOW);
}

GLES2_IDLOAD(glBlendColor);
GLES2_IDLOAD(glClearColor);
GLES2_IDLOAD(glClearDepthf);
GLES2_IDLOAD(glDepthRangef);
GLES2_IDLOAD(glLineWidth);
GLES2_IDLOAD(glPolygonOffset);
GLES2_IDLOAD(glSampleCoverage);
GLES2_IDLOAD(glTexParameterf);
GLES2_IDLOAD(glUniform1f);
GLES2_IDLOAD(glUniform2f);
GLES2_IDLOAD(glUniform3f);
GLES2_IDLOAD(glUniform4f);
GLES2_IDLOAD(glVertexAttrib1f);
GLES2_IDLOAD(glVertexAttrib2f);
GLES2_IDLOAD(glVertexAttrib3f);
GLES2_IDLOAD(glVertexAttrib4f);
GLES2_IDLOAD(glEGLImageTargetTexture2DOES);

GLES2_IDLOAD(glActiveTexture);

GLES2_IDLOAD(glAttachShader);

GLES2_IDLOAD(glBindAttribLocation);

GLES2_IDLOAD(glBindBuffer);

GLES2_IDLOAD(glBindFramebuffer);

GLES2_IDLOAD(glBindRenderbuffer);

GLES2_IDLOAD(glBindTexture);

GLES2_IDLOAD(glBlendEquation);

GLES2_IDLOAD(glBlendEquationSeparate);

GLES2_IDLOAD(glBlendFunc);

GLES2_IDLOAD(glBlendFuncSeparate);

GLES2_IDLOAD(glBufferData);

GLES2_IDLOAD(glBufferSubData);

GLES2_IDLOAD(glCheckFramebufferStatus);

GLES2_IDLOAD(glClear);

GLES2_IDLOAD(glClearStencil);

GLES2_IDLOAD(glColorMask);

GLES2_IDLOAD(glCompileShader);

GLES2_IDLOAD(glCompressedTexImage2D);

GLES2_IDLOAD(glCompressedTexSubImage2D);

GLES2_IDLOAD(glCopyTexImage2D);

GLES2_IDLOAD(glCopyTexSubImage2D);

GLES2_IDLOAD(glCreateProgram);

GLES2_IDLOAD(glCreateShader);

GLES2_IDLOAD(glCullFace);

GLES2_IDLOAD(glDeleteBuffers);

GLES2_IDLOAD(glDeleteFramebuffers);

GLES2_IDLOAD(glDeleteProgram);

GLES2_IDLOAD(glDeleteRenderbuffers);

GLES2_IDLOAD(glDeleteShader);

GLES2_IDLOAD(glDeleteTextures);

GLES2_IDLOAD(glDepthFunc);

GLES2_IDLOAD(glDepthMask);

GLES2_IDLOAD(glDetachShader);

GLES2_IDLOAD(glDisable);

GLES2_IDLOAD(glDisableVertexAttribArray);

GLES2_IDLOAD(glDrawArrays);

GLES2_IDLOAD(glDrawElements);

GLES2_IDLOAD(glEnable);

GLES2_IDLOAD(glEnableVertexAttribArray);

GLES2_IDLOAD(glFinish);

GLES2_IDLOAD(glFlush);

GLES2_IDLOAD(glFramebufferRenderbuffer);

GLES2_IDLOAD(glFramebufferTexture2D);

GLES2_IDLOAD(glFrontFace);

GLES2_IDLOAD(glGenBuffers);

GLES2_IDLOAD(glGenerateMipmap);

GLES2_IDLOAD(glGenFramebuffers);

GLES2_IDLOAD(glGenRenderbuffers);

GLES2_IDLOAD(glGenTextures);

GLES2_IDLOAD(glGetActiveAttrib);

GLES2_IDLOAD(glGetActiveUniform);

GLES2_IDLOAD(glGetAttachedShaders);

GLES2_IDLOAD(glGetAttribLocation);

GLES2_IDLOAD(glGetBooleanv);

GLES2_IDLOAD(glGetBufferParameteriv);

GLES2_IDLOAD(glGetError);

GLES2_IDLOAD(glGetFloatv);

GLES2_IDLOAD(glGetFramebufferAttachmentParameteriv);

GLES2_IDLOAD(glGetIntegerv);

GLES2_IDLOAD(glGetProgramiv);

GLES2_IDLOAD(glGetProgramInfoLog);

GLES2_IDLOAD(glGetRenderbufferParameteriv);

GLES2_IDLOAD(glGetShaderiv);

GLES2_IDLOAD(glGetShaderInfoLog);

GLES2_IDLOAD(glGetShaderPrecisionFormat);

GLES2_IDLOAD(glGetShaderSource);

GLES2_IDLOAD(glGetString);

GLES2_IDLOAD(glGetTexParameterfv);

GLES2_IDLOAD(glGetTexParameteriv);

GLES2_IDLOAD(glGetUniformfv);

GLES2_IDLOAD(glGetUniformiv);

GLES2_IDLOAD(glGetUniformLocation);

GLES2_IDLOAD(glGetVertexAttribfv);

GLES2_IDLOAD(glGetVertexAttribiv);

GLES2_IDLOAD(glGetVertexAttribPointerv);

GLES2_IDLOAD(glHint);

GLES2_IDLOAD(glIsBuffer);

GLES2_IDLOAD(glIsEnabled);

GLES2_IDLOAD(glIsFramebuffer);

GLES2_IDLOAD(glIsProgram);

GLES2_IDLOAD(glIsRenderbuffer);

GLES2_IDLOAD(glIsShader);

GLES2_IDLOAD(glIsTexture);

GLES2_IDLOAD(glLinkProgram);

GLES2_IDLOAD(glPixelStorei);

GLES2_IDLOAD(glReadPixels);

GLES2_IDLOAD(glReleaseShaderCompiler);

GLES2_IDLOAD(glRenderbufferStorage);

GLES2_IDLOAD(glScissor);

GLES2_IDLOAD(glShaderBinary);

GLES2_IDLOAD(glShaderSource);

GLES2_IDLOAD(glStencilFunc);

GLES2_IDLOAD(glStencilFuncSeparate);

GLES2_IDLOAD(glStencilMask);

GLES2_IDLOAD(glStencilMaskSeparate);

GLES2_IDLOAD(glStencilOp);

GLES2_IDLOAD(glStencilOpSeparate);

GLES2_IDLOAD(glTexImage2D);

GLES2_IDLOAD(glTexParameterfv);

GLES2_IDLOAD(glTexParameteri);

GLES2_IDLOAD(glTexParameteriv);

GLES2_IDLOAD(glTexSubImage2D);

GLES2_IDLOAD(glUniform1fv);

GLES2_IDLOAD(glUniform1i);

GLES2_IDLOAD(glUniform1iv);

GLES2_IDLOAD(glUniform2fv);

GLES2_IDLOAD(glUniform2i);

GLES2_IDLOAD(glUniform2iv);

GLES2_IDLOAD(glUniform3fv);

GLES2_IDLOAD(glUniform3i);

GLES2_IDLOAD(glUniform3iv);

GLES2_IDLOAD(glUniform4fv);

GLES2_IDLOAD(glUniform4i);

GLES2_IDLOAD(glUniform4iv);

GLES2_IDLOAD(glUniformMatrix2fv);

GLES2_IDLOAD(glUniformMatrix3fv);

GLES2_IDLOAD(glUniformMatrix4fv);

GLES2_IDLOAD(glUseProgram);

GLES2_IDLOAD(glValidateProgram);

GLES2_IDLOAD(glVertexAttrib1fv);

GLES2_IDLOAD(glVertexAttrib2fv);

GLES2_IDLOAD(glVertexAttrib3fv);

GLES2_IDLOAD(glVertexAttrib4fv);

GLES2_IDLOAD(glVertexAttribPointer);

GLES2_IDLOAD(glViewport);
