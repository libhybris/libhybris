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

#include <hybris/internal/binding.h>
#include <hybris/internal/floating_point_abi.h>

static void *_libglesv2 = NULL;

static void         (*_glActiveTexture)(GLenum texture) = NULL;
static void         (*_glAttachShader)(GLuint program, GLuint shader) = NULL;
static void         (*_glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name) = NULL;
static void         (*_glBindBuffer)(GLenum target, GLuint buffer) = NULL;
static void         (*_glBindFramebuffer)(GLenum target, GLuint framebuffer) = NULL;
static void         (*_glBindRenderbuffer)(GLenum target, GLuint renderbuffer) = NULL;
static void         (*_glBindTexture)(GLenum target, GLuint texture) = NULL;
static void         (*_glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) FP_ATTRIB = NULL;
static void         (*_glBlendEquation)(GLenum mode ) = NULL;
static void         (*_glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha) = NULL;
static void         (*_glBlendFunc)(GLenum sfactor, GLenum dfactor) = NULL;
static void         (*_glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) = NULL;
static void         (*_glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) = NULL;
static void         (*_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) = NULL;
static GLenum       (*_glCheckFramebufferStatus)(GLenum target) = NULL;
static void         (*_glClear)(GLbitfield mask) = NULL;
static void         (*_glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) FP_ATTRIB = NULL;
static void         (*_glClearDepthf)(GLclampf depth) FP_ATTRIB = NULL;
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
static void         (*_glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers) = NULL;
static void         (*_glDeleteProgram)(GLuint program) = NULL;
static void         (*_glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers) = NULL;
static void         (*_glDeleteShader)(GLuint shader) = NULL;
static void         (*_glDeleteTextures)(GLsizei n, const GLuint* textures) = NULL;
static void         (*_glDepthFunc)(GLenum func) = NULL;
static void         (*_glDepthMask)(GLboolean flag) = NULL;
static void         (*_glDepthRangef)(GLclampf zNear, GLclampf zFar) FP_ATTRIB = NULL;
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
static void         (*_glLineWidth)(GLfloat width) FP_ATTRIB = NULL;
static void         (*_glLinkProgram)(GLuint program) = NULL;
static void         (*_glPixelStorei)(GLenum pname, GLint param) = NULL;
static void         (*_glPolygonOffset)(GLfloat factor, GLfloat units) FP_ATTRIB = NULL;
static void         (*_glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels) = NULL;
static void         (*_glReleaseShaderCompiler)(void) = NULL;
static void         (*_glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
static void         (*_glSampleCoverage)(GLclampf value, GLboolean invert) FP_ATTRIB = NULL;
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
static void         (*_glTexParameterf)(GLenum target, GLenum pname, GLfloat param) FP_ATTRIB = NULL;
static void         (*_glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params) = NULL;
static void         (*_glTexParameteri)(GLenum target, GLenum pname, GLint param) = NULL;
static void         (*_glTexParameteriv)(GLenum target, GLenum pname, const GLint* params) = NULL;
static void         (*_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) = NULL;
static void         (*_glUniform1f)(GLint location, GLfloat x) FP_ATTRIB = NULL;
static void         (*_glUniform1fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform1i)(GLint location, GLint x) = NULL;
static void         (*_glUniform1iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform2f)(GLint location, GLfloat x, GLfloat y) FP_ATTRIB = NULL;
static void         (*_glUniform2fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform2i)(GLint location, GLint x, GLint y) = NULL;
static void         (*_glUniform2iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z) FP_ATTRIB = NULL;
static void         (*_glUniform3fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform3i)(GLint location, GLint x, GLint y, GLint z) = NULL;
static void         (*_glUniform3iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) FP_ATTRIB = NULL;
static void         (*_glUniform4fv)(GLint location, GLsizei count, const GLfloat* v) = NULL;
static void         (*_glUniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w) = NULL;
static void         (*_glUniform4iv)(GLint location, GLsizei count, const GLint* v) = NULL;
static void         (*_glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void         (*_glUseProgram)(GLuint program) = NULL;
static void         (*_glValidateProgram)(GLuint program) = NULL;
static void         (*_glVertexAttrib1f)(GLuint indx, GLfloat x) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib1fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib2fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib3fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) FP_ATTRIB = NULL;
static void         (*_glVertexAttrib4fv)(GLuint indx, const GLfloat* values) = NULL;
static void         (*_glVertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr) = NULL;
static void         (*_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static void         (*_glEGLImageTargetTexture2DOES) (GLenum target, GLeglImageOES image) = NULL;

static void _init_androidglesv2()
{
	_libglesv2 = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("libGLESv2") : "/system/lib/libGLESv2.so", RTLD_LAZY);
}


#define GLES2_DLSYM(sym) do { if (_libglesv2 == NULL) { _init_androidglesv2(); }; if (*(_ ## sym) == NULL) { *(&_ ## sym) = (void *) android_dlsym(_libglesv2, #sym); } } while (0) 

void glActiveTexture (GLenum texture)
{
	GLES2_DLSYM(glActiveTexture);

	return (*_glActiveTexture)(texture);
}

void glAttachShader (GLuint program, GLuint shader)
{
	GLES2_DLSYM(glAttachShader);

	return (*_glAttachShader)(program, shader);
}

void glBindAttribLocation (GLuint program, GLuint index, const GLchar* name)
{
	GLES2_DLSYM(glBindAttribLocation);

	return (*_glBindAttribLocation)(program, index, name);
}

void glBindBuffer (GLenum target, GLuint buffer)
{
	GLES2_DLSYM(glBindBuffer);

	return (*_glBindBuffer)(target, buffer);
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
	GLES2_DLSYM(glBindFramebuffer);

	return (*_glBindFramebuffer)(target, framebuffer);
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
	GLES2_DLSYM(glBindRenderbuffer);

	return (*_glBindRenderbuffer)(target, renderbuffer);
}

void glBindTexture (GLenum target, GLuint texture)
{
	GLES2_DLSYM(glBindTexture);

	return (*_glBindTexture)(target, texture);
}

void glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	GLES2_DLSYM(glBlendColor);

	return (*_glBlendColor)(red, green, blue, alpha);
}

void glBlendEquation ( GLenum mode )
{
	GLES2_DLSYM(glBlendEquation);

	return (*_glBlendEquation)(mode);
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
	GLES2_DLSYM(glBlendEquationSeparate);

	return (*_glBlendEquationSeparate)(modeRGB, modeAlpha);
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	GLES2_DLSYM(glBlendFunc);

	return (*_glBlendFunc)(sfactor, dfactor);
}

void glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	GLES2_DLSYM(glBlendFuncSeparate);

	return (*_glBlendFuncSeparate)(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBufferData (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	GLES2_DLSYM(glBufferData);

	return (*_glBufferData)(target, size, data, usage);
}

void glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	GLES2_DLSYM(glBufferSubData);

	return (*_glBufferSubData)(target, offset, size, data);
}

GLenum glCheckFramebufferStatus (GLenum target)
{
	GLES2_DLSYM(glCheckFramebufferStatus);

	return (*_glCheckFramebufferStatus)(target);
}

void glClear (GLbitfield mask)
{
	GLES2_DLSYM(glClear);

	return (*_glClear)(mask);
}

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	GLES2_DLSYM(glClearColor);
	return (*_glClearColor)(red, green, blue, alpha);
}

void glClearDepthf (GLclampf depth)
{
	GLES2_DLSYM(glClearDepthf);
	return (*_glClearDepthf)(depth);
}

void glClearStencil (GLint s)
{
	GLES2_DLSYM(glClearStencil);
	return (*_glClearStencil)(s);
}

void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	GLES2_DLSYM(glColorMask);
	return (*_glColorMask)(red, green, blue, alpha);
}

void glCompileShader (GLuint shader)
{
	GLES2_DLSYM(glCompileShader);
	return (*_glCompileShader)(shader);
}

void glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	GLES2_DLSYM(glCompressedTexImage2D);
	return (*_glCompressedTexImage2D)(target, level, internalformat, width, height, border, imageSize, data);
}

void glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data)
{
	GLES2_DLSYM(glCompressedTexSubImage2D);
	return (*_glCompressedTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	GLES2_DLSYM(glCopyTexImage2D);
	return (*_glCopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	GLES2_DLSYM(glCopyTexSubImage2D);
	return (*_glCopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

GLuint glCreateProgram (void)
{
	GLES2_DLSYM(glCreateProgram);
	return (*_glCreateProgram)();
}

GLuint glCreateShader (GLenum type)
{
	GLES2_DLSYM(glCreateShader);
	return (*_glCreateShader)(type);
}

void glCullFace (GLenum mode)
{
	GLES2_DLSYM(glCullFace);
	return (*_glCullFace)(mode);
}

void glDeleteBuffers (GLsizei n, const GLuint* buffers)
{
	GLES2_DLSYM(glDeleteBuffers);
	return (*_glDeleteBuffers)(n, buffers);
}

void glDeleteFramebuffers (GLsizei n, const GLuint* framebuffers)
{
	GLES2_DLSYM(glDeleteFramebuffers);
	return (*_glDeleteFramebuffers)(n, framebuffers);
}

void glDeleteProgram (GLuint program)
{
	GLES2_DLSYM(glDeleteProgram);
	return (*_glDeleteProgram)(program);
}

void glDeleteRenderbuffers (GLsizei n, const GLuint* renderbuffers)
{
	GLES2_DLSYM(glDeleteRenderbuffers);
	return (*_glDeleteRenderbuffers)(n, renderbuffers);
}

void glDeleteShader (GLuint shader)
{
	GLES2_DLSYM(glDeleteShader);
	return (*_glDeleteShader)(shader);
}

void glDeleteTextures (GLsizei n, const GLuint* textures)
{
	GLES2_DLSYM(glDeleteTextures);
	return (*_glDeleteTextures)(n, textures);
}

void glDepthFunc (GLenum func)
{
	GLES2_DLSYM(glDepthFunc);
	return (*_glDepthFunc)(func);
}

void glDepthMask (GLboolean flag)
{
	GLES2_DLSYM(glDepthMask);
	return (*_glDepthMask)(flag);
}

void glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	GLES2_DLSYM(glDepthRangef);
	return (*_glDepthRangef)(zNear, zFar);
}

void glDetachShader (GLuint program, GLuint shader)
{
	GLES2_DLSYM(glDetachShader);
	return (*_glDetachShader)(program, shader);
}

void glDisable (GLenum cap)
{
	GLES2_DLSYM(glDisable);
	return (*_glDisable)(cap);
}

void glDisableVertexAttribArray (GLuint index)
{
	GLES2_DLSYM(glDisableVertexAttribArray);
	return (*_glDisableVertexAttribArray)(index);
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	GLES2_DLSYM(glDrawArrays);
	return (*_glDrawArrays)(mode, first, count);
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	GLES2_DLSYM(glDrawElements);
	return (*_glDrawElements)(mode, count, type, indices);
}

void glEnable (GLenum cap)
{
	GLES2_DLSYM(glEnable);
	return (*_glEnable)(cap);
}

void glEnableVertexAttribArray (GLuint index)
{
	GLES2_DLSYM(glEnableVertexAttribArray);
	return (*_glEnableVertexAttribArray)(index);
}

void glFinish (void)
{
	GLES2_DLSYM(glFinish);
	return (*_glFinish)();
}

void glFlush (void)
{
	GLES2_DLSYM(glFlush);
	return (*_glFlush)();
}

void glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	GLES2_DLSYM(glFramebufferRenderbuffer);
	return (*_glFramebufferRenderbuffer)(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	GLES2_DLSYM(glFramebufferTexture2D);
	return (*_glFramebufferTexture2D)(target, attachment, textarget, texture, level);
}

void glFrontFace (GLenum mode)
{
	GLES2_DLSYM(glFrontFace);
	return (*_glFrontFace)(mode);
}

void glGenBuffers (GLsizei n, GLuint* buffers)
{
	GLES2_DLSYM(glGenBuffers);
	return (*_glGenBuffers)(n, buffers);
}

void glGenerateMipmap (GLenum target)
{
	GLES2_DLSYM(glGenerateMipmap);
	return (*_glGenerateMipmap)(target);
}

void glGenFramebuffers (GLsizei n, GLuint* framebuffers)
{
	GLES2_DLSYM(glGenFramebuffers);
	return (*_glGenFramebuffers)(n, framebuffers);
}

void glGenRenderbuffers (GLsizei n, GLuint* renderbuffers)
{
	GLES2_DLSYM(glGenRenderbuffers);
	return (*_glGenRenderbuffers)(n, renderbuffers);
}

void glGenTextures (GLsizei n, GLuint* textures)
{
	GLES2_DLSYM(glGenTextures);
	return (*_glGenTextures)(n, textures);
}

void glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	GLES2_DLSYM(glGetActiveAttrib);
	return (*_glGetActiveAttrib)(program, index, bufsize, length, size, type, name);
}

void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	GLES2_DLSYM(glGetActiveUniform);
	return (*_glGetActiveUniform)(program, index, bufsize, length, size, type, name);
}

void glGetAttachedShaders (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
	GLES2_DLSYM(glGetAttachedShaders);
	return (*_glGetAttachedShaders)(program, maxcount, count, shaders);
}

int glGetAttribLocation (GLuint program, const GLchar* name)
{
	GLES2_DLSYM(glGetAttribLocation);
	return (*_glGetAttribLocation)(program, name);
}

void glGetBooleanv (GLenum pname, GLboolean* params)
{
	GLES2_DLSYM(glGetBooleanv);
	return (*_glGetBooleanv)(pname, params);
}

void glGetBufferParameteriv (GLenum target, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetBufferParameteriv);
	return (*_glGetBufferParameteriv)(target, pname, params);
}

GLenum glGetError (void)
{
	GLES2_DLSYM(glGetError);
	return (*_glGetError)();
}

void glGetFloatv (GLenum pname, GLfloat* params)
{
	GLES2_DLSYM(glGetFloatv);
	return (*_glGetFloatv)(pname, params);
}

void glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetFramebufferAttachmentParameteriv);
	return (*_glGetFramebufferAttachmentParameteriv)(target, attachment, pname, params);
}

void glGetIntegerv (GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetIntegerv);
	return (*_glGetIntegerv)(pname, params);
}

void glGetProgramiv (GLuint program, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetProgramiv);
	return (*_glGetProgramiv)(program, pname, params);
}

void glGetProgramInfoLog (GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	GLES2_DLSYM(glGetProgramInfoLog);
	return (*_glGetProgramInfoLog)(program, bufsize, length, infolog);
}

void glGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetRenderbufferParameteriv);
	return (*_glGetRenderbufferParameteriv)(target, pname, params);
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetShaderiv);
	return (*_glGetShaderiv)(shader, pname, params);
}

void glGetShaderInfoLog (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	GLES2_DLSYM(glGetShaderInfoLog);
	return (*_glGetShaderInfoLog)(shader, bufsize, length, infolog);
}

void glGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	GLES2_DLSYM(glGetShaderPrecisionFormat);
	return (*_glGetShaderPrecisionFormat)(shadertype, precisiontype, range, precision);
}

void glGetShaderSource (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
	GLES2_DLSYM(glGetShaderSource);
	return (*_glGetShaderSource)(shader, bufsize, length, source);
}

const GLubyte* glGetString (GLenum name)
{
	GLES2_DLSYM(glGetString);
	return (*_glGetString)(name);
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat* params)
{
	GLES2_DLSYM(glGetTexParameterfv);
	return (*_glGetTexParameterfv)(target, pname, params);
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetTexParameteriv);
	return (*_glGetTexParameteriv)(target, pname, params);
}

void glGetUniformfv (GLuint program, GLint location, GLfloat* params)
{
	GLES2_DLSYM(glGetUniformfv);
	return (*_glGetUniformfv)(program, location, params);
}

void glGetUniformiv (GLuint program, GLint location, GLint* params)
{
	GLES2_DLSYM(glGetUniformiv);
	return (*_glGetUniformiv)(program, location, params);
}

int glGetUniformLocation (GLuint program, const GLchar* name)
{
	GLES2_DLSYM(glGetUniformLocation);
	return (*_glGetUniformLocation)(program, name);
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat* params)
{
	GLES2_DLSYM(glGetVertexAttribfv);
	return (*_glGetVertexAttribfv)(index, pname, params);
}

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint* params)
{
	GLES2_DLSYM(glGetVertexAttribiv);
	return (*_glGetVertexAttribiv)(index, pname, params);
}

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid** pointer)
{
	GLES2_DLSYM(glGetVertexAttribPointerv);
	return (*_glGetVertexAttribPointerv)(index, pname, pointer);
}

void glHint (GLenum target, GLenum mode)
{
	GLES2_DLSYM(glHint);
	return (*_glHint)(target, mode);
}

GLboolean glIsBuffer (GLuint buffer)
{
	GLES2_DLSYM(glIsBuffer);
	return (*_glIsBuffer)(buffer);
}

GLboolean glIsEnabled (GLenum cap)
{
	GLES2_DLSYM(glIsEnabled);
	return (*_glIsEnabled)(cap);
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
	GLES2_DLSYM(glIsFramebuffer);
	return (*_glIsFramebuffer)(framebuffer);
}

GLboolean glIsProgram (GLuint program)
{
	GLES2_DLSYM(glIsProgram);
	return (*_glIsProgram)(program);
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
	GLES2_DLSYM(glIsRenderbuffer);
	return (*_glIsRenderbuffer)(renderbuffer);
}

GLboolean glIsShader (GLuint shader)
{
	GLES2_DLSYM(glIsShader);
	return (*_glIsShader)(shader);
}

GLboolean glIsTexture (GLuint texture)
{
	GLES2_DLSYM(glIsTexture);
	return (*_glIsTexture)(texture);
}

void glLineWidth (GLfloat width)
{
	GLES2_DLSYM(glLineWidth);
	return (*_glLineWidth)(width);
}

void glLinkProgram (GLuint program)
{
	GLES2_DLSYM(glLinkProgram);
	return (*_glLinkProgram)(program);
}

void glPixelStorei (GLenum pname, GLint param)
{
	GLES2_DLSYM(glPixelStorei);
	return (*_glPixelStorei)(pname, param);
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
	GLES2_DLSYM(glPolygonOffset);
	return (*_glPolygonOffset)(factor, units);
}

void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	GLES2_DLSYM(glReadPixels);
	return (*_glReadPixels)(x, y, width, height, format, type, pixels);

}

void glReleaseShaderCompiler (void)
{
	GLES2_DLSYM(glReleaseShaderCompiler);
	return (*_glReleaseShaderCompiler)();
}

void glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	GLES2_DLSYM(glRenderbufferStorage);
	return (*_glRenderbufferStorage)(target, internalformat, width, height);
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
	GLES2_DLSYM(glSampleCoverage);
	return (*_glSampleCoverage)(value, invert);
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	GLES2_DLSYM(glScissor);
	return (*_glScissor)(x, y, width, height);
}

void glShaderBinary (GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
	GLES2_DLSYM(glShaderBinary);
	return (*_glShaderBinary)(n, shaders, binaryformat, binary, length);
}

void glShaderSource (GLuint shader, GLsizei count, const GLchar** string, const GLint* length)
{
	GLES2_DLSYM(glShaderSource);
	return (*_glShaderSource)(shader, count, string, length);
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	GLES2_DLSYM(glStencilFunc);
	return (*_glStencilFunc)(func, ref, mask);
}

void glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask)
{
	GLES2_DLSYM(glStencilFuncSeparate);
	return (*_glStencilFuncSeparate)(face, func, ref, mask);
}

void glStencilMask (GLuint mask)
{
	GLES2_DLSYM(glStencilMask);
	return (*_glStencilMask)(mask);
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
	GLES2_DLSYM(glStencilMaskSeparate);
	return (*_glStencilMaskSeparate)(face, mask);
}

void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	GLES2_DLSYM(glStencilOp);
	return (*_glStencilOp)(fail, zfail, zpass);
}

void glStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	GLES2_DLSYM(glStencilOpSeparate);
	return (*_glStencilOpSeparate)(face, fail, zfail, zpass);
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	GLES2_DLSYM(glTexImage2D);
	return (*_glTexImage2D)(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	GLES2_DLSYM(glTexParameterf);
	return (*_glTexParameterf)(target, pname, param);
}

void glTexParameterfv (GLenum target, GLenum pname, const GLfloat* params)
{
	GLES2_DLSYM(glTexParameterfv);
	return (*_glTexParameterfv)(target, pname, params);
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	GLES2_DLSYM(glTexParameteri);
	return (*_glTexParameteri)(target, pname, param);
}

void glTexParameteriv (GLenum target, GLenum pname, const GLint* params)
{
	GLES2_DLSYM(glTexParameteriv);
	return (*_glTexParameteriv)(target, pname, params);
}

void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	GLES2_DLSYM(glTexSubImage2D);
	return (*_glTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glUniform1f (GLint location, GLfloat x)
{
	GLES2_DLSYM(glUniform1f);
	return (*_glUniform1f)(location, x);
}

void glUniform1fv (GLint location, GLsizei count, const GLfloat* v)
{
	GLES2_DLSYM(glUniform1fv);
	return (*_glUniform1fv)(location, count, v);
}

void glUniform1i (GLint location, GLint x)
{
	GLES2_DLSYM(glUniform1i);
	return (*_glUniform1i)(location, x);
}

void glUniform1iv (GLint location, GLsizei count, const GLint* v)
{
	GLES2_DLSYM(glUniform1iv);
	return (*_glUniform1iv)(location, count, v);
}

void glUniform2f (GLint location, GLfloat x, GLfloat y)
{
	GLES2_DLSYM(glUniform2f);
	return (*_glUniform2f)(location, x, y);
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat* v)
{
	GLES2_DLSYM(glUniform2fv);
	return (*_glUniform2fv)(location, count, v);
}

void glUniform2i (GLint location, GLint x, GLint y)
{
	GLES2_DLSYM(glUniform2i);
	return (*_glUniform2i)(location, x, y);
}

void glUniform2iv (GLint location, GLsizei count, const GLint* v)
{
	GLES2_DLSYM(glUniform2iv);
	return (*_glUniform2iv)(location, count, v);
}

void glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
	GLES2_DLSYM(glUniform3f);
	return (*_glUniform3f)(location, x, y, z);
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat* v)
{
	GLES2_DLSYM(glUniform3fv);
	return (*_glUniform3fv)(location, count, v);
}

void glUniform3i (GLint location, GLint x, GLint y, GLint z)
{
	GLES2_DLSYM(glUniform3i);
	return (*_glUniform3i)(location, x, y, z);
}

void glUniform3iv (GLint location, GLsizei count, const GLint* v)
{
	GLES2_DLSYM(glUniform3iv);
	return (*_glUniform3iv)(location, count, v);
}

void glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLES2_DLSYM(glUniform4f);
	return (*_glUniform4f)(location, x, y, z, w);
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat* v)
{
	GLES2_DLSYM(glUniform4fv);
	return (*_glUniform4fv)(location, count, v);
}

void glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w)
{
	GLES2_DLSYM(glUniform4i);
	return (*_glUniform4i)(location, x, y, z, w);
}

void glUniform4iv (GLint location, GLsizei count, const GLint* v)
{
	GLES2_DLSYM(glUniform4iv);
	return (*_glUniform4iv)(location, count, v);
}

void glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	GLES2_DLSYM(glUniformMatrix2fv);
	return (*_glUniformMatrix2fv)(location, count, transpose, value);
}

void glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	GLES2_DLSYM(glUniformMatrix3fv);
	return (*_glUniformMatrix3fv)(location, count, transpose, value);
}

void glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	GLES2_DLSYM(glUniformMatrix4fv);
	return (*_glUniformMatrix4fv)(location, count, transpose, value);
}

void glUseProgram (GLuint program)
{
	GLES2_DLSYM(glUseProgram);
	return (*_glUseProgram)(program);
}

void glValidateProgram (GLuint program)
{
	GLES2_DLSYM(glValidateProgram);
	return (*_glValidateProgram)(program);
}

void glVertexAttrib1f (GLuint indx, GLfloat x)
{
	GLES2_DLSYM(glVertexAttrib1f);
	return (*_glVertexAttrib1f)(indx, x);
}

void glVertexAttrib1fv (GLuint indx, const GLfloat* values)
{
	GLES2_DLSYM(glVertexAttrib1fv);
	return (*_glVertexAttrib1fv)(indx, values);
}

void glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y)
{
	GLES2_DLSYM(glVertexAttrib2f);
	return (*_glVertexAttrib2f)(indx, x, y);
}

void glVertexAttrib2fv (GLuint indx, const GLfloat* values)
{
	GLES2_DLSYM(glVertexAttrib2fv);
	return (*_glVertexAttrib2fv)(indx, values);
}

void glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	GLES2_DLSYM(glVertexAttrib3f);
	return (*_glVertexAttrib3f)(indx, x, y, z);
}

void glVertexAttrib3fv (GLuint indx, const GLfloat* values)
{
	GLES2_DLSYM(glVertexAttrib3fv);
	return (*_glVertexAttrib3fv)(indx, values);
}

void glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLES2_DLSYM(glVertexAttrib4f);
	return (*_glVertexAttrib4f)(indx, x, y, z, w);
}

void glVertexAttrib4fv (GLuint indx, const GLfloat* values)
{
	GLES2_DLSYM(glVertexAttrib4fv);
	return (*_glVertexAttrib4fv)(indx, values);
}

void glVertexAttribPointer (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	GLES2_DLSYM(glVertexAttribPointer);
	return (*_glVertexAttribPointer)(indx, size, type, normalized, stride, ptr);
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	GLES2_DLSYM(glViewport);
	return (*_glViewport)(x, y, width, height);
}

void glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
	GLES2_DLSYM(glEGLImageTargetTexture2DOES);
	(*_glEGLImageTargetTexture2DOES)(target, image);
}



