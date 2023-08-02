/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "include/core/SkTypes.h"
#if defined(SK_BUILD_FOR_WIN)

#include "src/base/SkLeanWindows.h"

#include "include/gpu/gl/GrGLAssembleInterface.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "src/gpu/ganesh/gl/GrGLUtil.h"

#if defined(_M_ARM64)

sk_sp<const GrGLInterface> GrGLMakeNativeInterface() { return nullptr; }

#else

typedef HGLRC (WINAPI *WGLGetCurrentContextProc)(VOID);
typedef PROC (WINAPI *WGLGetProcAddressProc)(LPCSTR name);

class AutoLibraryUnload {
public:
    AutoLibraryUnload(const char* moduleName) {
        fModule = LoadLibraryA(moduleName);
    }
    ~AutoLibraryUnload() {
        if (fModule) {
            FreeLibrary(fModule);
        }
    }
    HMODULE get() const { return fModule; }

private:
    HMODULE fModule;
};

class GLProcGetter {
public:
    GLProcGetter() : fGLLib("opengl32.dll") {
        fGetProcAddress = (WGLGetProcAddressProc) GetProcAddress(fGLLib.get(), "wglGetProcAddress");
        fGetCurrentContext = (WGLGetCurrentContextProc) GetProcAddress(fGLLib.get(), "wglGetCurrentContext");
    }

    bool isInitialized() const {
        return SkToBool(fGLLib.get() && fGetCurrentContext && fGetProcAddress);
    }

    GrGLFuncPtr getProc(const char name[]) const {
        GrGLFuncPtr proc;
        if ((proc = (GrGLFuncPtr) GetProcAddress(fGLLib.get(), name))) {
            return proc;
        }
        if (fGetProcAddress && (proc = (GrGLFuncPtr) fGetProcAddress(name))) {
            return proc;
        }
        return nullptr;
    }

    void* getCurrentContext(void) const {
        if (!fGetCurrentContext)
            return nullptr;
        return fGetCurrentContext();
    }

private:
    AutoLibraryUnload fGLLib;
    WGLGetCurrentContextProc fGetCurrentContext;
    WGLGetProcAddressProc fGetProcAddress;
};

static GrGLFuncPtr win_get_gl_proc(void* ctx, const char name[]) {
    SkASSERT(ctx);
    const GLProcGetter* getter = (const GLProcGetter*) ctx;
    SkASSERT(getter->getCurrentContext());
    return getter->getProc(name);
}

/*
 * Windows makes the GL funcs all be __stdcall instead of __cdecl :(
 * This implementation will only work if GR_GL_FUNCTION_TYPE is __stdcall.
 * Otherwise, a springboard would be needed that hides the calling convention.
 */
sk_sp<const GrGLInterface> GrGLMakeNativeInterface() {
    GLProcGetter getter;
    if (!getter.isInitialized()) {
        return nullptr;
    }

    if (nullptr == getter.getCurrentContext()) {
        return nullptr;
    }

    GrGLGetStringFn* getString = (GrGLGetStringFn*)getter.getProc("glGetString");
    if (nullptr == getString) {
        return nullptr;
    }
    const char* verStr = reinterpret_cast<const char*>(getString(GR_GL_VERSION));
    GrGLStandard standard = GrGLGetStandardInUseFromString(verStr);

    if (GR_IS_GR_GL_ES(standard)) {
        return GrGLMakeAssembledGLESInterface(&getter, win_get_gl_proc);
    } else if (GR_IS_GR_GL(standard)) {
        return GrGLMakeAssembledGLInterface(&getter, win_get_gl_proc);
    }
    return nullptr;
}

#endif // ARM64

#endif//defined(SK_BUILD_FOR_WIN)
