#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdio.h>
#include "void.h"

void* void_library_load(const char *filepath) {
    if (!filepath) return NULL;
    void *handle = NULL;
#ifdef _WIN32
    handle = (void*)LoadLibraryA(filepath);
    if (!handle) VOID_LOG_ERROR("Failed to load library: %s (Win32 Error: %lu)", filepath, GetLastError());
#else
    handle = dlopen(filepath, RTLD_NOW | RTLD_LOCAL);
    if (!handle) VOID_LOG_ERROR("Failed to load library: %s (dlerror: %s)", filepath, dlerror());
#endif
    if (handle) VOID_LOG_INFO("Dynamic library loaded: %s", filepath);
    return handle;
}

void* void_library_get_symbol(void *handle, const char *symbol_name) {
    if (!handle || !symbol_name) return NULL;
    void *symbol = NULL;
#ifdef _WIN32
    symbol = (void*)GetProcAddress((HMODULE)handle, symbol_name);
    if (!symbol) VOID_LOG_ERROR("Failed to find symbol '%s' (Win32 Error: %lu)", symbol_name, GetLastError());
#else
    dlerror(); 
    symbol = dlsym(handle, symbol_name);
    const char *err = dlerror();
    if (err) VOID_LOG_ERROR("Failed to find symbol '%s' (dlerror: %s)", symbol_name, err);
#endif
    return symbol;
}

void void_library_free(void *handle) {
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
    VOID_LOG_INFO("Dynamic library freed.");
}
