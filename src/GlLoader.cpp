#include "GlLoader.h"

PFNGLCLEARPROC glClear = nullptr;
PFNGLCLEARCOLORPROC glClearColor = nullptr;
PFNGLVIEWPORTPROC glViewport = nullptr;
PFNGLENABLEPROC glEnable = nullptr;
PFNGLDISABLEPROC glDisable = nullptr;
PFNGLDEPTHFUNCPROC glDepthFunc = nullptr;
PFNGLBLENDFUNCPROC glBlendFunc = nullptr;
PFNGLCULLFACEPROC glCullFace = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glDrawElements = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM2FVPROC glUniform2fv = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORM4FVPROC glUniform4fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLGENTEXTURESPROC glGenTextures = nullptr;
PFNGLBINDTEXTUREPROC glBindTexture = nullptr;
PFNGLTEXIMAGE2DPROC glTexImage2D = nullptr;
PFNGLTEXPARAMETERIPROC glTexParameteri = nullptr;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLDELETETEXTURESPROC glDeleteTextures = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;

static void* getProc(GLADloadproc loader, const char* name) {
    return loader(name);
}

#define LOAD(fn, Type) fn = reinterpret_cast<Type>(getProc(loader, #fn))

int gladLoadGLLoader(GLADloadproc loader) {
    if (!loader) return 0;
    LOAD(glClear, PFNGLCLEARPROC);
    LOAD(glClearColor, PFNGLCLEARCOLORPROC);
    LOAD(glViewport, PFNGLVIEWPORTPROC);
    LOAD(glEnable, PFNGLENABLEPROC);
    LOAD(glDisable, PFNGLDISABLEPROC);
    LOAD(glDepthFunc, PFNGLDEPTHFUNCPROC);
    LOAD(glBlendFunc, PFNGLBLENDFUNCPROC);
    LOAD(glCullFace, PFNGLCULLFACEPROC);
    LOAD(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    LOAD(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    LOAD(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
    LOAD(glGenBuffers, PFNGLGENBUFFERSPROC);
    LOAD(glBindBuffer, PFNGLBINDBUFFERPROC);
    LOAD(glBufferData, PFNGLBUFFERDATAPROC);
    LOAD(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    LOAD(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    LOAD(glVertexAttribIPointer, PFNGLVERTEXATTRIBIPOINTERPROC);
    LOAD(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOAD(glDrawArrays, PFNGLDRAWARRAYSPROC);
    LOAD(glDrawElements, PFNGLDRAWELEMENTSPROC);
    LOAD(glCreateShader, PFNGLCREATESHADERPROC);
    LOAD(glShaderSource, PFNGLSHADERSOURCEPROC);
    LOAD(glCompileShader, PFNGLCOMPILESHADERPROC);
    LOAD(glGetShaderiv, PFNGLGETSHADERIVPROC);
    LOAD(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    LOAD(glDeleteShader, PFNGLDELETESHADERPROC);
    LOAD(glCreateProgram, PFNGLCREATEPROGRAMPROC);
    LOAD(glAttachShader, PFNGLATTACHSHADERPROC);
    LOAD(glLinkProgram, PFNGLLINKPROGRAMPROC);
    LOAD(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    LOAD(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    LOAD(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
    LOAD(glUseProgram, PFNGLUSEPROGRAMPROC);
    LOAD(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    LOAD(glUniform1i, PFNGLUNIFORM1IPROC);
    LOAD(glUniform1f, PFNGLUNIFORM1FPROC);
    LOAD(glUniform2fv, PFNGLUNIFORM2FVPROC);
    LOAD(glUniform3fv, PFNGLUNIFORM3FVPROC);
    LOAD(glUniform4fv, PFNGLUNIFORM4FVPROC);
    LOAD(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
    LOAD(glGenTextures, PFNGLGENTEXTURESPROC);
    LOAD(glBindTexture, PFNGLBINDTEXTUREPROC);
    LOAD(glTexImage2D, PFNGLTEXIMAGE2DPROC);
    LOAD(glTexParameteri, PFNGLTEXPARAMETERIPROC);
    LOAD(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);
    LOAD(glActiveTexture, PFNGLACTIVETEXTUREPROC);
    LOAD(glDeleteTextures, PFNGLDELETETEXTURESPROC);
    LOAD(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
    LOAD(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
    LOAD(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);
    LOAD(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
    LOAD(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
    return glCreateProgram != nullptr;
}

#undef LOAD

int gladLoadGL(GLADloadproc load) {
    return gladLoadGLLoader(load);
}
