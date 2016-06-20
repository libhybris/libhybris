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
#include <stdio.h>

#include <egl/ws.h>
#include <hybris/common/binding.h>
#include <hybris/common/floating_point_abi.h>

static void *_libglesv2 = NULL;

/* Only functions with floating point argument need a wrapper to change the call convention correctly */

static void         (*_glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) FP_ATTRIB = NULL;
static void         (*_glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) FP_ATTRIB = NULL;
static void         (*_glClearDepthf)(GLclampf depth) FP_ATTRIB = NULL;
static void         (*_glDepthRangef)(GLclampf zNear, GLclampf zFar) FP_ATTRIB = NULL;
static void         (*_glLineWidth)(GLfloat width) FP_ATTRIB = NULL;
static void         (*_glPolygonOffset)(GLfloat factor, GLfloat units) FP_ATTRIB = NULL;
static void         (*_glSampleCoverage)(GLclampf value, GLboolean invert) FP_ATTRIB = NULL;
static void         (*_glTexParameterf)(GLenum target, GLenum pname, GLfloat param) FP_ATTRIB = NULL;
static void         (*_glUniform1f)(GLint location, GLfloat x) FP_ATTRIB = NULL;
static void         (*_glUniform2f)(GLint location, GLfloat x, GLfloat y) FP_ATTRIB = NULL;
static void         (*_glUniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z) FP_ATTRIB = NULL;
static void         (*_glUniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib1f)(GLuint indx, GLfloat x) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) FP_ATTRIB = NULL;
static void         (*_glEGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image) = NULL;

static void         (*_glActiveTexture)(GLenum texture) = NULL;
static void         (*_glAttachShader)(GLuint program, GLuint shader) = NULL;
static void         (*_glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name) = NULL;
static void         (*_glBindBuffer)(GLenum target, GLuint buffer) = NULL;
static void         (*_glBindFramebuffer)(GLenum target, GLuint framebuffer) = NULL;
static void         (*_glBindRenderbuffer)(GLenum target, GLuint framebuffer) = NULL;
static void         (*_glBindTexture)(GLenum target, GLuint texture) = NULL;
static void         (*_glBlendEquation)(GLenum mode) = NULL;
static void         (*_glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha) = NULL;
static void         (*_glBlendFunc)(GLenum sfactor, GLenum dfactor) = NULL;
static void         (*_glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) = NULL;
static void         (*_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) = NULL;
static void         (*_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) = NULL;
static GLenum       (*_glCheckFramebufferStatus)(GLenum target) = NULL;
static void         (*_glClear)(GLbitfield mask) = NULL;
static void         (*_glClearStencil)(GLint s) = NULL;
static void         (*_glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = NULL;
static void         (*_glCompileShader)(GLuint shader) = NULL;
static void         (*_glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) = NULL;
static void         (*_glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data) = NULL;
static void         (*_glCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) = NULL;
static void         (*_glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static GLuint       (*_glCreateProgram)(void) = NULL;
static GLuint       (*_glCreateShader)(GLenum type) = NULL;
static void         (*_glCullFace)(GLenum mode) = NULL;
static void         (*_glDeleteBuffers)(GLsizei n, const GLuint* buffers) = NULL;
static void         (*_glDeleteFramebuffers)(GLsizei n, const GLuint* buffers) = NULL;
static void         (*_glDeleteProgram)(GLuint program) = NULL;
static void         (*_glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers) = NULL;
static void         (*_glDeleteShader)(GLuint shader) = NULL;
static void         (*_glDeleteTextures)(GLsizei n, const GLuint* textures) = NULL;
static void         (*_glDepthFunc)(GLenum func) = NULL;
static void         (*_glDepthMask)(GLboolean flag) = NULL;
static void         (*_glDetachShader)(GLuint program, GLuint shader) = NULL;
static void         (*_glDisable)(GLenum cap) = NULL;
static void         (*_glDisableVertexAttribArray)(GLuint index) = NULL;
static void         (*_glDrawArrays)(GLenum mode, GLint first, GLsizei count) = NULL;
static void         (*_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) = NULL;
static void         (*_glEnable)(GLenum cap) = NULL;
static void         (*_glEnableVertexAttribArray)(GLuint index) = NULL;
static void         (*_glFinish)(void) = NULL;
static void         (*_glFlush)(void) = NULL;
static void         (*_glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = NULL;
static void         (*_glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
static void         (*_glFrontFace)(GLenum mode) = NULL;
static void         (*_glGenBuffers)(GLsizei n, GLuint* buffers) = NULL;
static void         (*_glGenerateMipmap)(GLenum target) = NULL;
static void         (*_glGenFramebuffers)(GLsizei n, GLuint* framebuffers) = NULL;
static void         (*_glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers) = NULL;
static void         (*_glGenTextures)(GLsizei n, GLuint* textures) = NULL;
static void         (*_glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) = NULL;
static void         (*_glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) = NULL;
static void         (*_glGetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) = NULL;
static int          (*_glGetAttribLocation)(GLuint program, const GLchar* name) = NULL;
static void         (*_glGetBooleanv)(GLenum pname, GLboolean* params) = NULL;
static void         (*_glGetBufferParameteriv)(GLenum target, GLenum pname, GLint* params) = NULL;
static GLenum       (*_glGetError)(void) = NULL;
static void         (*_glGetFloatv)(GLenum pname, GLfloat* params) = NULL;
static void         (*_glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params) = NULL;
static void         (*_glGetIntegerv)(GLenum pname, GLint* params) = NULL;
static void         (*_glGetProgramiv)(GLuint program, GLenum pname, GLint* params) = NULL;
static void         (*_glGetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog) = NULL;
static void         (*_glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params) = NULL;
static void         (*_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params) = NULL;
static void         (*_glGetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog) = NULL;
static void         (*_glGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) = NULL;
static void         (*_glGetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source) = NULL;
static const GLubyte* (*_glGetString)(GLenum name) = NULL;
static void         (*_glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params) = NULL;
static void         (*_glGetTexParameteriv)(GLenum target, GLenum pname, GLint* params) = NULL;
static void         (*_glGetUniformfv)(GLuint program, GLint location, GLfloat* params) = NULL;
static void         (*_glGetUniformiv)(GLuint program, GLint location, GLint* params) = NULL;
static int          (*_glGetUniformLocation)(GLuint program, const GLchar* name) = NULL;
static void         (*_glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params) = NULL;
static void         (*_glGetVertexAttribiv)(GLuint index, GLenum pname, GLint* params) = NULL;
static void         (*_glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid** pointer) = NULL;
static void         (*_glHint)(GLenum target, GLenum mode) = NULL;
static GLboolean    (*_glIsBuffer)(GLuint buffer) = NULL;
static GLboolean    (*_glIsEnabled)(GLenum cap) = NULL;
static GLboolean    (*_glIsFramebuffer)(GLuint framebuffer) = NULL;
static GLboolean    (*_glIsProgram)(GLuint program) = NULL;
static GLboolean    (*_glIsRenderbuffer)(GLuint renderbuffer) = NULL;
static GLboolean    (*_glIsShader)(GLuint shader) = NULL;
static GLboolean    (*_glIsTexture)(GLuint texture) = NULL;
static void         (*_glLinkProgram)(GLuint program) = NULL;
static void         (*_glPixelStorei)(GLenum pname, GLint param) = NULL;
static void         (*_glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels) = NULL;
static void         (*_glReleaseShaderCompiler)(void) = NULL;
static void         (*_glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
static void         (*_glScissor)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static void         (*_glShaderBinary)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length) = NULL;
static void         (*_glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) = NULL;
static void         (*_glStencilFunc)(GLenum func, GLint ref, GLuint mask) = NULL;
static void         (*_glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask) = NULL;
static void         (*_glStencilMask)(GLuint mask) = NULL;
static void         (*_glStencilMaskSeparate)(GLenum face, GLuint mask) = NULL;
static void         (*_glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass) = NULL;
static void         (*_glStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass) = NULL;
static void         (*_glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) = NULL;
static void         (*_glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params) = NULL;
static void         (*_glTexParameteri)(GLenum target, GLenum pname, GLint param) = NULL;
static void         (*_glTexParameteriv)(GLenum target, GLenum pname, const GLint* params) = NULL;
static void         (*_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) = NULL;
static void         (*_glUniform1fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform1i)(GLint location, GLint x) = NULL;
static void         (*_glUniform1iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform2fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform2i)(GLint location, GLint x, GLint y) = NULL;
static void         (*_glUniform2iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform3fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform3i)(GLint location, GLint x, GLint y, GLint z) = NULL;
static void         (*_glUniform3iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform4fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w) = NULL;
static void         (*_glUniform4iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUseProgram)(GLuint program) = NULL;
static void         (*_glValidateProgram)(GLuint program) = NULL;
static void         (*_glVertexAttrib1fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib2fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib3fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib4fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr) = NULL;
static void         (*_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;


#define GLES2_LOAD(sym)  { *(&_ ## sym) = (void *) android_dlsym(_libglesv2, #sym);  } 

/*
This generates a function that when first called overwrites it's plt entry with new address. Subsequent calls jump directly at the target function in the android library. This means effectively 0 call overhead after the first call.
*/

#define GLES2_IDLOAD(sym) \
 __asm__ (".type " #sym ", %gnu_indirect_function"); \
typeof(sym) * sym ## _dispatch (void) __asm__ (#sym);\
typeof(sym) * sym ## _dispatch (void) \
{ \
	if (_libglesv2) \
		return (void *) android_dlsym(_libglesv2, #sym); \
	else \
		return &sym ## _wrapper; \
}

static void  __attribute__((constructor)) _init_androidglesv2()  {
	_libglesv2 = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("LIBGLESV2") : "libGLESv2.so", RTLD_NOW);
	GLES2_LOAD(glBlendColor);
	GLES2_LOAD(glClearColor);
	GLES2_LOAD(glClearDepthf);
	GLES2_LOAD(glDepthRangef);
	GLES2_LOAD(glLineWidth);
	GLES2_LOAD(glPolygonOffset);
	GLES2_LOAD(glSampleCoverage);
	GLES2_LOAD(glTexParameterf);
	GLES2_LOAD(glUniform1f);
	GLES2_LOAD(glUniform2f);
	GLES2_LOAD(glUniform3f);
	GLES2_LOAD(glUniform4f);
	GLES2_LOAD(glVertexAttrib1f);
	GLES2_LOAD(glVertexAttrib2f);
	GLES2_LOAD(glVertexAttrib3f);
	GLES2_LOAD(glVertexAttrib4f);
	GLES2_LOAD(glEGLImageTargetTexture2DOES);

	GLES2_LOAD(glActiveTexture);
	GLES2_LOAD(glAttachShader);
	GLES2_LOAD(glBindAttribLocation);
	GLES2_LOAD(glBindBuffer);
	GLES2_LOAD(glBindFramebuffer);
	GLES2_LOAD(glBindRenderbuffer);
	GLES2_LOAD(glBindTexture);
	GLES2_LOAD(glBlendEquation);
	GLES2_LOAD(glBlendEquationSeparate);
	GLES2_LOAD(glBlendFunc);
	GLES2_LOAD(glBlendFuncSeparate);
	GLES2_LOAD(glBufferData);
	GLES2_LOAD(glBufferSubData);
	GLES2_LOAD(glCheckFramebufferStatus);
	GLES2_LOAD(glClear);
	GLES2_LOAD(glClearStencil);
	GLES2_LOAD(glColorMask);
	GLES2_LOAD(glCompileShader);
	GLES2_LOAD(glCompressedTexImage2D);
	GLES2_LOAD(glCompressedTexSubImage2D);
	GLES2_LOAD(glCopyTexImage2D);
	GLES2_LOAD(glCopyTexSubImage2D);
	GLES2_LOAD(glCreateProgram);
	GLES2_LOAD(glCreateShader);
	GLES2_LOAD(glCullFace);
	GLES2_LOAD(glDeleteBuffers);
	GLES2_LOAD(glDeleteFramebuffers);
	GLES2_LOAD(glDeleteProgram);
	GLES2_LOAD(glDeleteRenderbuffers);
	GLES2_LOAD(glDeleteShader);
	GLES2_LOAD(glDeleteTextures);
	GLES2_LOAD(glDepthFunc);
	GLES2_LOAD(glDepthMask);
	GLES2_LOAD(glDetachShader);
	GLES2_LOAD(glDisable);
	GLES2_LOAD(glDisableVertexAttribArray);
	GLES2_LOAD(glDrawArrays);
	GLES2_LOAD(glDrawElements);
	GLES2_LOAD(glEnable);
	GLES2_LOAD(glEnableVertexAttribArray);
	GLES2_LOAD(glFinish);
	GLES2_LOAD(glFlush);
	GLES2_LOAD(glFramebufferRenderbuffer);
	GLES2_LOAD(glFramebufferTexture2D);
	GLES2_LOAD(glFrontFace);
	GLES2_LOAD(glGenBuffers);
	GLES2_LOAD(glGenerateMipmap);
	GLES2_LOAD(glGenFramebuffers);
	GLES2_LOAD(glGenRenderbuffers);
	GLES2_LOAD(glGenTextures);
	GLES2_LOAD(glGetActiveAttrib);
	GLES2_LOAD(glGetActiveUniform);
	GLES2_LOAD(glGetAttachedShaders);
	GLES2_LOAD(glGetAttribLocation);
	GLES2_LOAD(glGetBooleanv);
	GLES2_LOAD(glGetBufferParameteriv);
	GLES2_LOAD(glGetError);
	GLES2_LOAD(glGetFloatv);
	GLES2_LOAD(glGetFramebufferAttachmentParameteriv);
	GLES2_LOAD(glGetIntegerv);
	GLES2_LOAD(glGetProgramiv);
	GLES2_LOAD(glGetProgramInfoLog);
	GLES2_LOAD(glGetRenderbufferParameteriv);
	GLES2_LOAD(glGetShaderiv);
	GLES2_LOAD(glGetShaderInfoLog);
	GLES2_LOAD(glGetShaderPrecisionFormat);
	GLES2_LOAD(glGetShaderSource);
	GLES2_LOAD(glGetString);
	GLES2_LOAD(glGetTexParameterfv);
	GLES2_LOAD(glGetTexParameteriv);
	GLES2_LOAD(glGetUniformfv);
	GLES2_LOAD(glGetUniformiv);
	GLES2_LOAD(glGetUniformLocation);
	GLES2_LOAD(glGetVertexAttribfv);
	GLES2_LOAD(glGetVertexAttribiv);
	GLES2_LOAD(glGetVertexAttribPointerv);
	GLES2_LOAD(glHint);
	GLES2_LOAD(glIsBuffer);
	GLES2_LOAD(glIsEnabled);
	GLES2_LOAD(glIsFramebuffer);
	GLES2_LOAD(glIsProgram);
	GLES2_LOAD(glIsRenderbuffer);
	GLES2_LOAD(glIsShader);
	GLES2_LOAD(glIsTexture);
	GLES2_LOAD(glLinkProgram);
	GLES2_LOAD(glPixelStorei);
	GLES2_LOAD(glReadPixels);
	GLES2_LOAD(glReleaseShaderCompiler);
	GLES2_LOAD(glRenderbufferStorage);
	GLES2_LOAD(glScissor);
	GLES2_LOAD(glShaderBinary);
	GLES2_LOAD(glShaderSource);
	GLES2_LOAD(glStencilFunc);
	GLES2_LOAD(glStencilFuncSeparate);
	GLES2_LOAD(glStencilMask);
	GLES2_LOAD(glStencilMaskSeparate);
	GLES2_LOAD(glStencilOp);
	GLES2_LOAD(glStencilOpSeparate);
	GLES2_LOAD(glTexImage2D);
	GLES2_LOAD(glTexParameterfv);
	GLES2_LOAD(glTexParameteri);
	GLES2_LOAD(glTexParameteriv);
	GLES2_LOAD(glTexSubImage2D);
	GLES2_LOAD(glUniform1fv);
	GLES2_LOAD(glUniform1i);
	GLES2_LOAD(glUniform1iv);
	GLES2_LOAD(glUniform2fv);
	GLES2_LOAD(glUniform2i);
	GLES2_LOAD(glUniform2iv);
	GLES2_LOAD(glUniform3fv);
	GLES2_LOAD(glUniform3i);
	GLES2_LOAD(glUniform3iv);
	GLES2_LOAD(glUniform4fv);
	GLES2_LOAD(glUniform4i);
	GLES2_LOAD(glUniform4iv);
	GLES2_LOAD(glUniformMatrix2fv);
	GLES2_LOAD(glUniformMatrix3fv);
	GLES2_LOAD(glUniformMatrix4fv);
	GLES2_LOAD(glUseProgram);
	GLES2_LOAD(glValidateProgram);
	GLES2_LOAD(glVertexAttrib1fv);
	GLES2_LOAD(glVertexAttrib2fv);
	GLES2_LOAD(glVertexAttrib3fv);
	GLES2_LOAD(glVertexAttrib4fv);
	GLES2_LOAD(glVertexAttribPointer);
	GLES2_LOAD(glViewport);
}

static void glActiveTexture_wrapper(GLenum texture)
{
	return (*_glActiveTexture)(texture);
}

static void glAttachShader_wrapper(GLuint program, GLuint shader)
{
	return (*_glAttachShader)(program, shader);
}

static void glBindAttribLocation_wrapper(GLuint program, GLuint index, const GLchar* name)
{
	return (*_glBindAttribLocation)(program, index, name);
}

static void glBindBuffer_wrapper(GLenum target, GLuint buffer)
{
	return (*_glBindBuffer)(target, buffer);
}

static void glBindFramebuffer_wrapper(GLenum target, GLuint framebuffer)
{
	return (*_glBindFramebuffer)(target, framebuffer);
}

static void glBindRenderbuffer_wrapper(GLenum target, GLuint framebuffer)
{
	return (*_glBindRenderbuffer)(target, framebuffer);
}

static void glBindTexture_wrapper(GLenum target, GLuint texture)
{
	return (*_glBindTexture)(target, texture);
}

static void glBlendEquation_wrapper(GLenum mode)
{
	return (*_glBlendEquation)(mode);
}

static void glBlendEquationSeparate_wrapper(GLenum modeRGB, GLenum modeAlpha)
{
	return (*_glBlendEquationSeparate)(modeRGB, modeAlpha);
}

static void glBlendFunc_wrapper(GLenum sfactor, GLenum dfactor)
{
	return (*_glBlendFunc)(sfactor, dfactor);
}

static void glBlendFuncSeparate_wrapper(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	return (*_glBlendFuncSeparate)(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

static void glBufferData_wrapper(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	return (*_glBufferData)(target, size, data, usage);
}

static void glBufferSubData_wrapper(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	return (*_glBufferSubData)(target, offset, size, data);
}

static GLenum glCheckFramebufferStatus_wrapper(GLenum target)
{
	return (*_glCheckFramebufferStatus)(target);
}

static void glClear_wrapper(GLbitfield mask)
{
	return (*_glClear)(mask);
}

static void glClearStencil_wrapper(GLint s)
{
	return (*_glClearStencil)(s);
}

static void glColorMask_wrapper(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	return (*_glColorMask)(red, green, blue, alpha);
}

static void glCompileShader_wrapper(GLuint shader)
{
	return (*_glCompileShader)(shader);
}

static void glCompressedTexImage2D_wrapper(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	return (*_glCompressedTexImage2D)(target, level, internalformat, width, height, border, imageSize, data);
}

static void glCompressedTexSubImage2D_wrapper(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data)
{
	return (*_glCompressedTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static void glCopyTexImage2D_wrapper(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	return (*_glCopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

static void glCopyTexSubImage2D_wrapper(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glCopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

static GLuint glCreateProgram_wrapper(void)
{
	return (*_glCreateProgram)();
}

static GLuint glCreateShader_wrapper(GLenum type)
{
	return (*_glCreateShader)(type);
}

static void glCullFace_wrapper(GLenum mode)
{
	return (*_glCullFace)(mode);
}

static void glDeleteBuffers_wrapper(GLsizei n, const GLuint* buffers)
{
	return (*_glDeleteBuffers)(n, buffers);
}

static void glDeleteFramebuffers_wrapper(GLsizei n, const GLuint* buffers)
{
	return (*_glDeleteFramebuffers)(n, buffers);
}

static void glDeleteProgram_wrapper(GLuint program)
{
	return (*_glDeleteProgram)(program);
}

static void glDeleteRenderbuffers_wrapper(GLsizei n, const GLuint* renderbuffers)
{
	return (*_glDeleteRenderbuffers)(n, renderbuffers);
}

static void glDeleteShader_wrapper(GLuint shader)
{
	return (*_glDeleteShader)(shader);
}

static void glDeleteTextures_wrapper(GLsizei n, const GLuint* textures)
{
	return (*_glDeleteTextures)(n, textures);
}

static void glDepthFunc_wrapper(GLenum func)
{
	return (*_glDepthFunc)(func);
}

static void glDepthMask_wrapper(GLboolean flag)
{
	return (*_glDepthMask)(flag);
}

static void glDetachShader_wrapper(GLuint program, GLuint shader)
{
	return (*_glDetachShader)(program, shader);
}

static void glDisable_wrapper(GLenum cap)
{
	return (*_glDisable)(cap);
}

static void glDisableVertexAttribArray_wrapper(GLuint index)
{
	return (*_glDisableVertexAttribArray)(index);
}

static void glDrawArrays_wrapper(GLenum mode, GLint first, GLsizei count)
{
	return (*_glDrawArrays)(mode, first, count);
}

static void glDrawElements_wrapper(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	return (*_glDrawElements)(mode, count, type, indices);
}

static void glEnable_wrapper(GLenum cap)
{
	return (*_glEnable)(cap);
}

static void glEnableVertexAttribArray_wrapper(GLuint index)
{
	return (*_glEnableVertexAttribArray)(index);
}

static void glFinish_wrapper(void)
{
	return (*_glFinish)();
}

static void glFlush_wrapper(void)
{
	return (*_glFlush)();
}

static void glFramebufferRenderbuffer_wrapper(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	return (*_glFramebufferRenderbuffer)(target, attachment, renderbuffertarget, renderbuffer);
}

static void glFramebufferTexture2D_wrapper(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	return (*_glFramebufferTexture2D)(target, attachment, textarget, texture, level);
}

static void glFrontFace_wrapper(GLenum mode)
{
	return (*_glFrontFace)(mode);
}

static void glGenBuffers_wrapper(GLsizei n, GLuint* buffers)
{
	return (*_glGenBuffers)(n, buffers);
}

static void glGenerateMipmap_wrapper(GLenum target)
{
	return (*_glGenerateMipmap)(target);
}

static void glGenFramebuffers_wrapper(GLsizei n, GLuint* framebuffers)
{
	return (*_glGenFramebuffers)(n, framebuffers);
}

static void glGenRenderbuffers_wrapper(GLsizei n, GLuint* renderbuffers)
{
	return (*_glGenRenderbuffers)(n, renderbuffers);
}

static void glGenTextures_wrapper(GLsizei n, GLuint* textures)
{
	return (*_glGenTextures)(n, textures);
}

static void glGetActiveAttrib_wrapper(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	return (*_glGetActiveAttrib)(program, index, bufsize, length, size, type, name);
}

static void glGetActiveUniform_wrapper(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	return (*_glGetActiveUniform)(program, index, bufsize, length, size, type, name);
}

static void glGetAttachedShaders_wrapper(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
	return (*_glGetAttachedShaders)(program, maxcount, count, shaders);
}

static int  glGetAttribLocation_wrapper(GLuint program, const GLchar* name)
{
	return (*_glGetAttribLocation)(program, name);
}

static void glGetBooleanv_wrapper(GLenum pname, GLboolean* params)
{
	return (*_glGetBooleanv)(pname, params);
}

static void glGetBufferParameteriv_wrapper(GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetBufferParameteriv)(target, pname, params);
}

static GLenum glGetError_wrapper(void)
{
	return (*_glGetError)();
}

static void glGetFloatv_wrapper(GLenum pname, GLfloat* params)
{
	return (*_glGetFloatv)(pname, params);
}

static void glGetFramebufferAttachmentParameteriv_wrapper(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	return (*_glGetFramebufferAttachmentParameteriv)(target, attachment, pname, params);
}

static void glGetIntegerv_wrapper(GLenum pname, GLint* params)
{
	return (*_glGetIntegerv)(pname, params);
}

static void glGetProgramiv_wrapper(GLuint program, GLenum pname, GLint* params)
{
	return (*_glGetProgramiv)(program, pname, params);
}

static void glGetProgramInfoLog_wrapper(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return (*_glGetProgramInfoLog)(program, bufsize, length, infolog);
}

static void glGetRenderbufferParameteriv_wrapper(GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetRenderbufferParameteriv)(target, pname, params);
}

static void glGetShaderiv_wrapper(GLuint shader, GLenum pname, GLint* params)
{
	return (*_glGetShaderiv)(shader, pname, params);
}

static void glGetShaderInfoLog_wrapper(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return (*_glGetShaderInfoLog)(shader, bufsize, length, infolog);
}

static void glGetShaderPrecisionFormat_wrapper(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	return (*_glGetShaderPrecisionFormat)(shadertype, precisiontype, range, precision);
}

static void glGetShaderSource_wrapper(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
	return (*_glGetShaderSource)(shader, bufsize, length, source);
}

static void glGetTexParameterfv_wrapper(GLenum target, GLenum pname, GLfloat* params)
{
	return (*_glGetTexParameterfv)(target, pname, params);
}

static void glGetTexParameteriv_wrapper(GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetTexParameteriv)(target, pname, params);
}

static void glGetUniformfv_wrapper(GLuint program, GLint location, GLfloat* params)
{
	return (*_glGetUniformfv)(program, location, params);
}

static void glGetUniformiv_wrapper(GLuint program, GLint location, GLint* params)
{
	return (*_glGetUniformiv)(program, location, params);
}

static int  glGetUniformLocation_wrapper(GLuint program, const GLchar* name)
{
	return (*_glGetUniformLocation)(program, name);
}

static void glGetVertexAttribfv_wrapper(GLuint index, GLenum pname, GLfloat* params)
{
	return (*_glGetVertexAttribfv)(index, pname, params);
}

static void glGetVertexAttribiv_wrapper(GLuint index, GLenum pname, GLint* params)
{
	return (*_glGetVertexAttribiv)(index, pname, params);
}

static void glGetVertexAttribPointerv_wrapper(GLuint index, GLenum pname, GLvoid** pointer)
{
	return (*_glGetVertexAttribPointerv)(index, pname, pointer);
}

static void glHint_wrapper(GLenum target, GLenum mode)
{
	return (*_glHint)(target, mode);
}

static GLboolean glIsBuffer_wrapper(GLuint buffer)
{
	return (*_glIsBuffer)(buffer);
}

static GLboolean glIsEnabled_wrapper(GLenum cap)
{
	return (*_glIsEnabled)(cap);
}

static GLboolean glIsFramebuffer_wrapper(GLuint framebuffer)
{
	return (*_glIsFramebuffer)(framebuffer);
}

static GLboolean glIsProgram_wrapper(GLuint program)
{
	return (*_glIsProgram)(program);
}

static GLboolean glIsRenderbuffer_wrapper(GLuint renderbuffer)
{
	return (*_glIsRenderbuffer)(renderbuffer);
}

static GLboolean glIsShader_wrapper(GLuint shader)
{
	return (*_glIsShader)(shader);
}

static GLboolean glIsTexture_wrapper(GLuint texture)
{
	return (*_glIsTexture)(texture);
}

static void glLinkProgram_wrapper(GLuint program)
{
	return (*_glLinkProgram)(program);
}

static void glPixelStorei_wrapper(GLenum pname, GLint param)
{
	return (*_glPixelStorei)(pname, param);
}

static void glReadPixels_wrapper(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	return (*_glReadPixels)(x, y, width, height, format, type, pixels);
}

static void glReleaseShaderCompiler_wrapper(void)
{
	return (*_glReleaseShaderCompiler)();
}

static void glRenderbufferStorage_wrapper(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	return (*_glRenderbufferStorage)(target, internalformat, width, height);
}

static void glScissor_wrapper(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glScissor)(x, y, width, height);
}

static void glShaderBinary_wrapper(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
	return (*_glShaderBinary)(n, shaders, binaryformat, binary, length);
}

static void glShaderSource_wrapper(GLuint shader, GLsizei count, const GLchar** string, const GLint* length)
{
	return (*_glShaderSource)(shader, count, string, length);
}

static void glStencilFunc_wrapper(GLenum func, GLint ref, GLuint mask)
{
	return (*_glStencilFunc)(func, ref, mask);
}

static void glStencilFuncSeparate_wrapper(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	return (*_glStencilFuncSeparate)(face, func, ref, mask);
}

static void glStencilMask_wrapper(GLuint mask)
{
	return (*_glStencilMask)(mask);
}

static void glStencilMaskSeparate_wrapper(GLenum face, GLuint mask)
{
	return (*_glStencilMaskSeparate)(face, mask);
}

static void glStencilOp_wrapper(GLenum fail, GLenum zfail, GLenum zpass)
{
	return (*_glStencilOp)(fail, zfail, zpass);
}

static void glStencilOpSeparate_wrapper(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	return (*_glStencilOpSeparate)(face, fail, zfail, zpass);
}

static void glTexImage2D_wrapper(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	return (*_glTexImage2D)(target, level, internalformat, width, height, border, format, type, pixels);
}

static void glTexParameterfv_wrapper(GLenum target, GLenum pname, const GLfloat* params)
{
	return (*_glTexParameterfv)(target, pname, params);
}

static void glTexParameteri_wrapper(GLenum target, GLenum pname, GLint param)
{
	return (*_glTexParameteri)(target, pname, param);
}

static void glTexParameteriv_wrapper(GLenum target, GLenum pname, const GLint* params)
{
	return (*_glTexParameteriv)(target, pname, params);
}

static void glTexSubImage2D_wrapper(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	return (*_glTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void glUniform1fv_wrapper(GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform1fv)(location, count, v);
}

static void glUniform1i_wrapper(GLint location, GLint x)
{
	return (*_glUniform1i)(location, x);
}

static void glUniform1iv_wrapper(GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform1iv)(location, count, v);
}

static void glUniform2fv_wrapper(GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform2fv)(location, count, v);
}

static void glUniform2i_wrapper(GLint location, GLint x, GLint y)
{
	return (*_glUniform2i)(location, x, y);
}

static void glUniform2iv_wrapper(GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform2iv)(location, count, v);
}

static void glUniform3fv_wrapper(GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform3fv)(location, count, v);
}

static void glUniform3i_wrapper(GLint location, GLint x, GLint y, GLint z)
{
	return (*_glUniform3i)(location, x, y, z);
}

static void glUniform3iv_wrapper(GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform3iv)(location, count, v);
}

static void glUniform4fv_wrapper(GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform4fv)(location, count, v);
}

static void glUniform4i_wrapper(GLint location, GLint x, GLint y, GLint z, GLint w)
{
	return (*_glUniform4i)(location, x, y, z, w);
}

static void glUniform4iv_wrapper(GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform4iv)(location, count, v);
}

static void glUniformMatrix2fv_wrapper(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix2fv)(location, count, transpose, value);
}

static void glUniformMatrix3fv_wrapper(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix3fv)(location, count, transpose, value);
}

static void glUniformMatrix4fv_wrapper(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix4fv)(location, count, transpose, value);
}

static void glUseProgram_wrapper(GLuint program)
{
	return (*_glUseProgram)(program);
}

static void glValidateProgram_wrapper(GLuint program)
{
	return (*_glValidateProgram)(program);
}

static void glVertexAttrib1fv_wrapper(GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib1fv)(indx, values);
}

static void glVertexAttrib2fv_wrapper(GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib2fv)(indx, values);
}

static void glVertexAttrib3fv_wrapper(GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib3fv)(indx, values);
}

static void glVertexAttrib4fv_wrapper(GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib4fv)(indx, values);
}

static void glVertexAttribPointer_wrapper(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	return (*_glVertexAttribPointer)(indx, size, type, normalized, stride, ptr);
}

static void glViewport_wrapper(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glViewport)(x, y, width, height);
}



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

void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
	struct egl_image *img = image;
	return (*_glEGLImageTargetTexture2DOES)(target, img ? img->egl_image : NULL);
}

void glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return (*_glBlendColor)(red, green, blue, alpha);
}

void glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return (*_glVertexAttrib4f)(indx, x, y, z, w);
}

void glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y)
{
	return (*_glVertexAttrib2f)(indx, x, y);
}

void glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	return (*_glVertexAttrib3f)(indx, x, y, z);
}


void glVertexAttrib1f (GLuint indx, GLfloat x)
{
	return (*_glVertexAttrib1f)(indx, x);
}

void glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return (*_glUniform4f)(location, x, y, z, w);
}

void glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
	return (*_glUniform3f)(location, x, y, z);
}

void glUniform2f (GLint location, GLfloat x, GLfloat y)
{
	return (*_glUniform2f)(location, x, y);
}

void glUniform1f (GLint location, GLfloat x)
{
	return (*_glUniform1f)(location, x);
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	return (*_glTexParameterf)(target, pname, param);
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
	return (*_glSampleCoverage)(value, invert);
}
void glPolygonOffset (GLfloat factor, GLfloat units)
{
	return (*_glPolygonOffset)(factor, units);
}
void glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	return (*_glDepthRangef)(zNear, zFar);
}


void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return (*_glClearColor)(red, green, blue, alpha);
}

void glClearDepthf (GLclampf depth)
{
	return (*_glClearDepthf)(depth);
}
void glLineWidth (GLfloat width)
{
	return (*_glLineWidth)(width);
}

const GLubyte *glGetString(GLenum name)
{
    // Return 2.0 even though drivers might actually support 3.0 or higher,
    // because libhybris does not provide any 3.0+ symbols.
    if (name == GL_VERSION) {
        static GLubyte glGetString_versionString[64];
        snprintf(glGetString_versionString, sizeof(glGetString_versionString), "OpenGL ES 2.0 (%s)", (*_glGetString)(name));
        return glGetString_versionString;
    }
	return (*_glGetString)(name);
}
