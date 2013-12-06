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


#define GLES2_LOAD(sym)  { *(&_ ## sym) = (void *) android_dlsym(_libglesv2, #sym);  } 

static void  __attribute__((constructor)) _init_androidglesv2()  {
	_libglesv2 = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("LIBGLESV2") : "libGLESv2.so", RTLD_NOW);
	GLES2_LOAD(glActiveTexture);
	GLES2_LOAD(glAttachShader);
	GLES2_LOAD(glBindAttribLocation);
	GLES2_LOAD(glBindBuffer);
	GLES2_LOAD(glBindFramebuffer);
	GLES2_LOAD(glBindRenderbuffer);
	GLES2_LOAD(glBindTexture);
	GLES2_LOAD(glBlendColor);
	GLES2_LOAD(glBlendEquation);
	GLES2_LOAD(glBlendEquationSeparate);
	GLES2_LOAD(glBlendFunc);
	GLES2_LOAD(glBlendFuncSeparate);
	GLES2_LOAD(glBufferData);
	GLES2_LOAD(glBufferSubData);
	GLES2_LOAD(glCheckFramebufferStatus);
	GLES2_LOAD(glClear);
	GLES2_LOAD(glClearColor);
	GLES2_LOAD(glClearDepthf);
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
	GLES2_LOAD(glDepthRangef);
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
	GLES2_LOAD(glLineWidth);
	GLES2_LOAD(glLinkProgram);
	GLES2_LOAD(glPixelStorei);
	GLES2_LOAD(glPolygonOffset);
	GLES2_LOAD(glReadPixels);
	GLES2_LOAD(glReleaseShaderCompiler);
	GLES2_LOAD(glRenderbufferStorage);
	GLES2_LOAD(glSampleCoverage);
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
	GLES2_LOAD(glTexParameterf);
	GLES2_LOAD(glTexParameterfv);
	GLES2_LOAD(glTexParameteri);
	GLES2_LOAD(glTexParameteriv);
	GLES2_LOAD(glTexSubImage2D);
	GLES2_LOAD(glUniform1f);
	GLES2_LOAD(glUniform1fv);
	GLES2_LOAD(glUniform1i);
	GLES2_LOAD(glUniform1iv);
	GLES2_LOAD(glUniform2f);
	GLES2_LOAD(glUniform2fv);
	GLES2_LOAD(glUniform2i);
	GLES2_LOAD(glUniform2iv);
	GLES2_LOAD(glUniform3f);
	GLES2_LOAD(glUniform3fv);
	GLES2_LOAD(glUniform3i);
	GLES2_LOAD(glUniform3iv);
	GLES2_LOAD(glUniform4f);
	GLES2_LOAD(glUniform4fv);
	GLES2_LOAD(glUniform4i);
	GLES2_LOAD(glUniform4iv);
	GLES2_LOAD(glUniformMatrix2fv);
	GLES2_LOAD(glUniformMatrix3fv);
	GLES2_LOAD(glUniformMatrix4fv);
	GLES2_LOAD(glUseProgram);
	GLES2_LOAD(glValidateProgram);
	GLES2_LOAD(glVertexAttrib1f);
	GLES2_LOAD(glVertexAttrib1fv);
	GLES2_LOAD(glVertexAttrib2f);
	GLES2_LOAD(glVertexAttrib2fv);
	GLES2_LOAD(glVertexAttrib3f);
	GLES2_LOAD(glVertexAttrib3fv);
	GLES2_LOAD(glVertexAttrib4f);
	GLES2_LOAD(glVertexAttrib4fv);
	GLES2_LOAD(glVertexAttribPointer);
	GLES2_LOAD(glViewport);
	GLES2_LOAD(glEGLImageTargetTexture2DOES);
	
}


void glActiveTexture (GLenum texture)
{
	return (*_glActiveTexture)(texture);
}

void glAttachShader (GLuint program, GLuint shader)
{
	return (*_glAttachShader)(program, shader);
}

void glBindAttribLocation (GLuint program, GLuint index, const GLchar* name)
{
	return (*_glBindAttribLocation)(program, index, name);
}

void glBindBuffer (GLenum target, GLuint buffer)
{
	return (*_glBindBuffer)(target, buffer);
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
	return (*_glBindFramebuffer)(target, framebuffer);
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
	return (*_glBindRenderbuffer)(target, renderbuffer);
}

void glBindTexture (GLenum target, GLuint texture)
{
	return (*_glBindTexture)(target, texture);
}

void glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return (*_glBlendColor)(red, green, blue, alpha);
}

void glBlendEquation ( GLenum mode )
{
	return (*_glBlendEquation)(mode);
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
	return (*_glBlendEquationSeparate)(modeRGB, modeAlpha);
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	return (*_glBlendFunc)(sfactor, dfactor);
}

void glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	return (*_glBlendFuncSeparate)(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void glBufferData (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	return (*_glBufferData)(target, size, data, usage);
}

void glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	return (*_glBufferSubData)(target, offset, size, data);
}

GLenum glCheckFramebufferStatus (GLenum target)
{
	return (*_glCheckFramebufferStatus)(target);
}

void glClear (GLbitfield mask)
{
	return (*_glClear)(mask);
}

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return (*_glClearColor)(red, green, blue, alpha);
}

void glClearDepthf (GLclampf depth)
{
	return (*_glClearDepthf)(depth);
}

void glClearStencil (GLint s)
{
	return (*_glClearStencil)(s);
}

void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	return (*_glColorMask)(red, green, blue, alpha);
}

void glCompileShader (GLuint shader)
{
	return (*_glCompileShader)(shader);
}

void glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data)
{
	return (*_glCompressedTexImage2D)(target, level, internalformat, width, height, border, imageSize, data);
}

void glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data)
{
	return (*_glCompressedTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	return (*_glCopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glCopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

GLuint glCreateProgram (void)
{
	return (*_glCreateProgram)();
}

GLuint glCreateShader (GLenum type)
{
	return (*_glCreateShader)(type);
}

void glCullFace (GLenum mode)
{
	return (*_glCullFace)(mode);
}

void glDeleteBuffers (GLsizei n, const GLuint* buffers)
{
	return (*_glDeleteBuffers)(n, buffers);
}

void glDeleteFramebuffers (GLsizei n, const GLuint* framebuffers)
{
	return (*_glDeleteFramebuffers)(n, framebuffers);
}

void glDeleteProgram (GLuint program)
{
	return (*_glDeleteProgram)(program);
}

void glDeleteRenderbuffers (GLsizei n, const GLuint* renderbuffers)
{
	return (*_glDeleteRenderbuffers)(n, renderbuffers);
}

void glDeleteShader (GLuint shader)
{
	return (*_glDeleteShader)(shader);
}

void glDeleteTextures (GLsizei n, const GLuint* textures)
{
	return (*_glDeleteTextures)(n, textures);
}

void glDepthFunc (GLenum func)
{
	return (*_glDepthFunc)(func);
}

void glDepthMask (GLboolean flag)
{
	return (*_glDepthMask)(flag);
}

void glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	return (*_glDepthRangef)(zNear, zFar);
}

void glDetachShader (GLuint program, GLuint shader)
{
	return (*_glDetachShader)(program, shader);
}

void glDisable (GLenum cap)
{
	return (*_glDisable)(cap);
}

void glDisableVertexAttribArray (GLuint index)
{
	return (*_glDisableVertexAttribArray)(index);
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	return (*_glDrawArrays)(mode, first, count);
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	return (*_glDrawElements)(mode, count, type, indices);
}

void glEnable (GLenum cap)
{
	return (*_glEnable)(cap);
}

void glEnableVertexAttribArray (GLuint index)
{
	return (*_glEnableVertexAttribArray)(index);
}

void glFinish (void)
{
	return (*_glFinish)();
}

void glFlush (void)
{
	return (*_glFlush)();
}

void glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	return (*_glFramebufferRenderbuffer)(target, attachment, renderbuffertarget, renderbuffer);
}

void glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	return (*_glFramebufferTexture2D)(target, attachment, textarget, texture, level);
}

void glFrontFace (GLenum mode)
{
	return (*_glFrontFace)(mode);
}

void glGenBuffers (GLsizei n, GLuint* buffers)
{
	return (*_glGenBuffers)(n, buffers);
}

void glGenerateMipmap (GLenum target)
{
	return (*_glGenerateMipmap)(target);
}

void glGenFramebuffers (GLsizei n, GLuint* framebuffers)
{
	return (*_glGenFramebuffers)(n, framebuffers);
}

void glGenRenderbuffers (GLsizei n, GLuint* renderbuffers)
{
	return (*_glGenRenderbuffers)(n, renderbuffers);
}

void glGenTextures (GLsizei n, GLuint* textures)
{
	return (*_glGenTextures)(n, textures);
}

void glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	return (*_glGetActiveAttrib)(program, index, bufsize, length, size, type, name);
}

void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	return (*_glGetActiveUniform)(program, index, bufsize, length, size, type, name);
}

void glGetAttachedShaders (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
	return (*_glGetAttachedShaders)(program, maxcount, count, shaders);
}

int glGetAttribLocation (GLuint program, const GLchar* name)
{
	return (*_glGetAttribLocation)(program, name);
}

void glGetBooleanv (GLenum pname, GLboolean* params)
{
	return (*_glGetBooleanv)(pname, params);
}

void glGetBufferParameteriv (GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetBufferParameteriv)(target, pname, params);
}

GLenum glGetError (void)
{
	return (*_glGetError)();
}

void glGetFloatv (GLenum pname, GLfloat* params)
{
	return (*_glGetFloatv)(pname, params);
}

void glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	return (*_glGetFramebufferAttachmentParameteriv)(target, attachment, pname, params);
}

void glGetIntegerv (GLenum pname, GLint* params)
{
	return (*_glGetIntegerv)(pname, params);
}

void glGetProgramiv (GLuint program, GLenum pname, GLint* params)
{
	return (*_glGetProgramiv)(program, pname, params);
}

void glGetProgramInfoLog (GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return (*_glGetProgramInfoLog)(program, bufsize, length, infolog);
}

void glGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetRenderbufferParameteriv)(target, pname, params);
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint* params)
{
	return (*_glGetShaderiv)(shader, pname, params);
}

void glGetShaderInfoLog (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return (*_glGetShaderInfoLog)(shader, bufsize, length, infolog);
}

void glGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	return (*_glGetShaderPrecisionFormat)(shadertype, precisiontype, range, precision);
}

void glGetShaderSource (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
	return (*_glGetShaderSource)(shader, bufsize, length, source);
}

const GLubyte* glGetString (GLenum name)
{
	return (*_glGetString)(name);
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat* params)
{
	return (*_glGetTexParameterfv)(target, pname, params);
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint* params)
{
	return (*_glGetTexParameteriv)(target, pname, params);
}

void glGetUniformfv (GLuint program, GLint location, GLfloat* params)
{
	return (*_glGetUniformfv)(program, location, params);
}

void glGetUniformiv (GLuint program, GLint location, GLint* params)
{
	return (*_glGetUniformiv)(program, location, params);
}

int glGetUniformLocation (GLuint program, const GLchar* name)
{
	return (*_glGetUniformLocation)(program, name);
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat* params)
{
	return (*_glGetVertexAttribfv)(index, pname, params);
}

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint* params)
{
	return (*_glGetVertexAttribiv)(index, pname, params);
}

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid** pointer)
{
	return (*_glGetVertexAttribPointerv)(index, pname, pointer);
}

void glHint (GLenum target, GLenum mode)
{
	return (*_glHint)(target, mode);
}

GLboolean glIsBuffer (GLuint buffer)
{
	return (*_glIsBuffer)(buffer);
}

GLboolean glIsEnabled (GLenum cap)
{
	return (*_glIsEnabled)(cap);
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
	return (*_glIsFramebuffer)(framebuffer);
}

GLboolean glIsProgram (GLuint program)
{
	return (*_glIsProgram)(program);
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
	return (*_glIsRenderbuffer)(renderbuffer);
}

GLboolean glIsShader (GLuint shader)
{
	return (*_glIsShader)(shader);
}

GLboolean glIsTexture (GLuint texture)
{
	return (*_glIsTexture)(texture);
}

void glLineWidth (GLfloat width)
{
	return (*_glLineWidth)(width);
}

void glLinkProgram (GLuint program)
{
	return (*_glLinkProgram)(program);
}

void glPixelStorei (GLenum pname, GLint param)
{
	return (*_glPixelStorei)(pname, param);
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
	return (*_glPolygonOffset)(factor, units);
}

void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	return (*_glReadPixels)(x, y, width, height, format, type, pixels);

}

void glReleaseShaderCompiler (void)
{
	return (*_glReleaseShaderCompiler)();
}

void glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	return (*_glRenderbufferStorage)(target, internalformat, width, height);
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
	return (*_glSampleCoverage)(value, invert);
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glScissor)(x, y, width, height);
}

void glShaderBinary (GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
	return (*_glShaderBinary)(n, shaders, binaryformat, binary, length);
}

void glShaderSource (GLuint shader, GLsizei count, const GLchar** string, const GLint* length)
{
	return (*_glShaderSource)(shader, count, string, length);
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	return (*_glStencilFunc)(func, ref, mask);
}

void glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask)
{
	return (*_glStencilFuncSeparate)(face, func, ref, mask);
}

void glStencilMask (GLuint mask)
{
	return (*_glStencilMask)(mask);
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
	return (*_glStencilMaskSeparate)(face, mask);
}

void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	return (*_glStencilOp)(fail, zfail, zpass);
}

void glStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	return (*_glStencilOpSeparate)(face, fail, zfail, zpass);
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	return (*_glTexImage2D)(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	return (*_glTexParameterf)(target, pname, param);
}

void glTexParameterfv (GLenum target, GLenum pname, const GLfloat* params)
{
	return (*_glTexParameterfv)(target, pname, params);
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	return (*_glTexParameteri)(target, pname, param);
}

void glTexParameteriv (GLenum target, GLenum pname, const GLint* params)
{
	return (*_glTexParameteriv)(target, pname, params);
}

void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	return (*_glTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glUniform1f (GLint location, GLfloat x)
{
	return (*_glUniform1f)(location, x);
}

void glUniform1fv (GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform1fv)(location, count, v);
}

void glUniform1i (GLint location, GLint x)
{
	return (*_glUniform1i)(location, x);
}

void glUniform1iv (GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform1iv)(location, count, v);
}

void glUniform2f (GLint location, GLfloat x, GLfloat y)
{
	return (*_glUniform2f)(location, x, y);
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform2fv)(location, count, v);
}

void glUniform2i (GLint location, GLint x, GLint y)
{
	return (*_glUniform2i)(location, x, y);
}

void glUniform2iv (GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform2iv)(location, count, v);
}

void glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
	return (*_glUniform3f)(location, x, y, z);
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform3fv)(location, count, v);
}

void glUniform3i (GLint location, GLint x, GLint y, GLint z)
{
	return (*_glUniform3i)(location, x, y, z);
}

void glUniform3iv (GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform3iv)(location, count, v);
}

void glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return (*_glUniform4f)(location, x, y, z, w);
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat* v)
{
	return (*_glUniform4fv)(location, count, v);
}

void glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w)
{
	return (*_glUniform4i)(location, x, y, z, w);
}

void glUniform4iv (GLint location, GLsizei count, const GLint* v)
{
	return (*_glUniform4iv)(location, count, v);
}

void glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix2fv)(location, count, transpose, value);
}

void glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix3fv)(location, count, transpose, value);
}

void glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return (*_glUniformMatrix4fv)(location, count, transpose, value);
}

void glUseProgram (GLuint program)
{
	return (*_glUseProgram)(program);
}

void glValidateProgram (GLuint program)
{
	return (*_glValidateProgram)(program);
}

void glVertexAttrib1f (GLuint indx, GLfloat x)
{
	return (*_glVertexAttrib1f)(indx, x);
}

void glVertexAttrib1fv (GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib1fv)(indx, values);
}

void glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y)
{
	return (*_glVertexAttrib2f)(indx, x, y);
}

void glVertexAttrib2fv (GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib2fv)(indx, values);
}

void glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	return (*_glVertexAttrib3f)(indx, x, y, z);
}

void glVertexAttrib3fv (GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib3fv)(indx, values);
}

void glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return (*_glVertexAttrib4f)(indx, x, y, z, w);
}

void glVertexAttrib4fv (GLuint indx, const GLfloat* values)
{
	return (*_glVertexAttrib4fv)(indx, values);
}

void glVertexAttribPointer (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	return (*_glVertexAttribPointer)(indx, size, type, normalized, stride, ptr);
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	return (*_glViewport)(x, y, width, height);
}

void glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
	(*_glEGLImageTargetTexture2DOES)(target, image);
}


