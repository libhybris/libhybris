/*
 * Copyright (C) 2017 Bhushan Shah
 * Contact: Bhushan Shah <bshah@kde.org>
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
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>

#include <hybris/common/binding.h>

#define GLESV3_LIBRARY_PATH "libGLESv3.so"

HYBRIS_LIBRARY_INITIALIZE(glesv3, GLESV3_LIBRARY_PATH);

HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glActiveTexture, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glAttachShader, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glBindAttribLocation, GLuint, GLuint, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindBuffer, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindFramebuffer, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindRenderbuffer, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindTexture, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glBlendColor, GLfloat, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glBlendEquation, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBlendEquationSeparate, GLenum, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBlendFunc, GLenum, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glBlendFuncSeparate, GLenum, GLenum, GLenum, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glBufferData, GLenum, GLsizeiptr, void *, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glBufferSubData, GLenum, GLintptr, GLsizeiptr, void *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLenum, glCheckFramebufferStatus, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glClear, GLbitfield);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glClearColor, GLfloat, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glClearDepthf, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glClearStencil, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glColorMask, GLboolean, GLboolean, GLboolean, GLboolean);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glCompileShader, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION8(glesv3, glCompressedTexImage2D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION9(glesv3, glCompressedTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION8(glesv3, glCopyTexImage2D, GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION8(glesv3, glCopyTexSubImage2D, GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_FUNCTION0(glesv3, GLuint, glCreateProgram);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLuint, glCreateShader, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glCullFace, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteBuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteFramebuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDeleteProgram, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteRenderbuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDeleteShader, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteTextures, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDepthFunc, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDepthMask, GLboolean);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDepthRangef, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDetachShader, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDisable, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDisableVertexAttribArray, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glDrawArrays, GLenum, GLint, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glDrawElements, GLenum, GLsizei, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glEnable, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glEnableVertexAttribArray, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glFinish);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glFlush);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glFramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glFramebufferTexture2D, GLenum, GLenum, GLenum, GLuint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glFrontFace, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenBuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glGenerateMipmap, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenFramebuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenRenderbuffers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenTextures, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION7(glesv3, glGetActiveAttrib, GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION7(glesv3, glGetActiveUniform, GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetAttachedShaders, GLuint, GLsizei, GLsizei *, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, GLint, glGetAttribLocation, GLuint, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGetBooleanv, GLenum, GLboolean *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetBufferParameteriv, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_FUNCTION0(glesv3, GLenum, glGetError);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGetFloatv, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetFramebufferAttachmentParameteriv, GLenum, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGetIntegerv, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetProgramiv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetProgramInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetRenderbufferParameteriv, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetShaderiv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetShaderInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetShaderPrecisionFormat, GLenum, GLenum, GLint *, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetShaderSource, GLuint, GLsizei, GLsizei *, GLchar *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, const GLubyte *, glGetString, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetTexParameterfv, GLenum, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetTexParameteriv, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetUniformfv, GLuint, GLint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetUniformiv, GLuint, GLint, GLint *);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, GLint, glGetUniformLocation, GLuint, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetVertexAttribfv, GLuint, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetVertexAttribiv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetVertexAttribPointerv, GLuint, GLenum, void **);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glHint, GLenum, GLenum);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsBuffer, GLuint);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsEnabled, GLenum);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsFramebuffer, GLuint);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsProgram, GLuint);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsRenderbuffer, GLuint);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsShader, GLuint);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsTexture, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glLineWidth, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glLinkProgram, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glPixelStorei, GLenum, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glPolygonOffset, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION7(glesv3, glReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glReleaseShaderCompiler);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glRenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glSampleCoverage, GLfloat, GLboolean);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glScissor, GLint, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glShaderBinary, GLsizei, GLuint *, GLenum, void *, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glShaderSource, GLuint, GLsizei, GLchar * *, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glStencilFunc, GLenum, GLint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glStencilFuncSeparate, GLenum, GLenum, GLint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glStencilMask, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glStencilMaskSeparate, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glStencilOp, GLenum, GLenum, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glStencilOpSeparate, GLenum, GLenum, GLenum, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION9(glesv3, glTexImage2D, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glTexParameterf, GLenum, GLenum, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glTexParameterfv, GLenum, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glTexParameteri, GLenum, GLenum, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glTexParameteriv, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION9(glesv3, glTexSubImage2D, GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glUniform1f, GLint, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform1fv, GLint, GLsizei, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glUniform1i, GLint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform1iv, GLint, GLsizei, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2f, GLint, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2fv, GLint, GLsizei, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2i, GLint, GLint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2iv, GLint, GLsizei, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniform3f, GLint, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform3fv, GLint, GLsizei, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniform3i, GLint, GLint, GLint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform3iv, GLint, GLsizei, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glUniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform4fv, GLint, GLsizei, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glUniform4i, GLint, GLint, GLint, GLint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform4iv, GLint, GLsizei, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix2fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix3fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix4fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glUseProgram, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glValidateProgram, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttrib1f, GLuint, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttrib1fv, GLuint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glVertexAttrib2f, GLuint, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttrib2fv, GLuint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glVertexAttrib3f, GLuint, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttrib3fv, GLuint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glVertexAttrib4f, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttrib4fv, GLuint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION6(glesv3, glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glViewport, GLint, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glReadBuffer, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION6(glesv3, glDrawRangeElements, GLenum, GLuint, GLuint, GLsizei, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION10(glesv3, glTexImage3D, GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION11(glesv3, glTexSubImage3D, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION9(glesv3, glCopyTexSubImage3D, GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION9(glesv3, glCompressedTexImage3D, GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION11(glesv3, glCompressedTexSubImage3D, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenQueries, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteQueries, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsQuery, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBeginQuery, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glEndQuery, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetQueryiv, GLenum, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetQueryObjectuiv, GLuint, GLenum, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glUnmapBuffer, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetBufferPointerv, GLenum, GLenum, void **);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDrawBuffers, GLsizei, GLenum *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix2x3fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix3x2fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix2x4fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix4x2fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix3x4fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniformMatrix4x3fv, GLint, GLsizei, GLboolean, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION10(glesv3, glBlitFramebuffer, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glRenderbufferStorageMultisample, GLenum, GLsizei, GLenum, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glFramebufferTextureLayer, GLenum, GLenum, GLuint, GLint, GLint);
HYBRIS_IMPLEMENT_FUNCTION4(glesv3, void *, glMapBufferRange, GLenum, GLintptr, GLsizeiptr, GLbitfield);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glFlushMappedBufferRange, GLenum, GLintptr, GLsizeiptr);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glBindVertexArray, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteVertexArrays, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenVertexArrays, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsVertexArray, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetIntegeri_v, GLenum, GLuint, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glBeginTransformFeedback, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glEndTransformFeedback);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glBindBufferRange, GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glBindBufferBase, GLenum, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glTransformFeedbackVaryings, GLuint, GLsizei, GLchar * *, GLenum);
HYBRIS_IMPLEMENT_VOID_FUNCTION7(glesv3, glGetTransformFeedbackVarying, GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glVertexAttribIPointer, GLuint, GLint, GLenum, GLsizei, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetVertexAttribIiv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetVertexAttribIuiv, GLuint, GLenum, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glVertexAttribI4i, GLuint, GLint, GLint, GLint, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glVertexAttribI4ui, GLuint, GLuint, GLuint, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttribI4iv, GLuint, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttribI4uiv, GLuint, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetUniformuiv, GLuint, GLint, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, GLint, glGetFragDataLocation, GLuint, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glUniform1ui, GLint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2ui, GLint, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glUniform3ui, GLint, GLuint, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glUniform4ui, GLint, GLuint, GLuint, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform1uiv, GLint, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform2uiv, GLint, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform3uiv, GLint, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniform4uiv, GLint, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glClearBufferiv, GLenum, GLint, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glClearBufferuiv, GLenum, GLint, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glClearBufferfv, GLenum, GLint, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glClearBufferfi, GLenum, GLint, GLfloat, GLint);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, const GLubyte *, glGetStringi, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glCopyBufferSubData, GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetUniformIndices, GLuint, GLsizei, GLchar * *, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glGetActiveUniformsiv, GLuint, GLsizei, GLuint *, GLenum, GLint *);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, GLuint, glGetUniformBlockIndex, GLuint, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glGetActiveUniformBlockiv, GLuint, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glGetActiveUniformBlockName, GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glUniformBlockBinding, GLuint, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glDrawArraysInstanced, GLenum, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glDrawElementsInstanced, GLenum, GLsizei, GLenum, void *, GLsizei);
HYBRIS_IMPLEMENT_FUNCTION2(glesv3, GLsync, glFenceSync, GLenum, GLbitfield);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsSync, GLsync);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(glesv3, glDeleteSync, GLsync);
HYBRIS_IMPLEMENT_FUNCTION3(glesv3, GLenum, glClientWaitSync, GLsync, GLbitfield, GLuint64);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glWaitSync, GLsync, GLbitfield, GLuint64);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGetInteger64v, GLenum, GLint64 *);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glGetSynciv, GLsync, GLenum, GLsizei, GLsizei *, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetInteger64i_v, GLenum, GLuint, GLint64 *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetBufferParameteri64v, GLenum, GLenum, GLint64 *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenSamplers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteSamplers, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsSampler, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindSampler, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glSamplerParameteri, GLuint, GLenum, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glSamplerParameteriv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glSamplerParameterf, GLuint, GLenum, GLfloat);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glSamplerParameterfv, GLuint, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetSamplerParameteriv, GLuint, GLenum, GLint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glGetSamplerParameterfv, GLuint, GLenum, GLfloat *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glVertexAttribDivisor, GLuint, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glBindTransformFeedback, GLenum, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glDeleteTransformFeedbacks, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_VOID_FUNCTION2(glesv3, glGenTransformFeedbacks, GLsizei, GLuint *);
HYBRIS_IMPLEMENT_FUNCTION1(glesv3, GLboolean, glIsTransformFeedback, GLuint);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glPauseTransformFeedback);
HYBRIS_IMPLEMENT_VOID_FUNCTION0(glesv3, glResumeTransformFeedback);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glGetProgramBinary, GLuint, GLsizei, GLsizei *, GLenum *, void *);
HYBRIS_IMPLEMENT_VOID_FUNCTION4(glesv3, glProgramBinary, GLuint, GLenum, void *, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glProgramParameteri, GLuint, GLenum, GLint);
HYBRIS_IMPLEMENT_VOID_FUNCTION3(glesv3, glInvalidateFramebuffer, GLenum, GLsizei, GLenum *);
HYBRIS_IMPLEMENT_VOID_FUNCTION7(glesv3, glInvalidateSubFramebuffer, GLenum, GLsizei, GLenum *, GLint, GLint, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glTexStorage2D, GLenum, GLsizei, GLenum, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION6(glesv3, glTexStorage3D, GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
HYBRIS_IMPLEMENT_VOID_FUNCTION5(glesv3, glGetInternalformativ, GLenum, GLenum, GLenum, GLsizei, GLint *);
