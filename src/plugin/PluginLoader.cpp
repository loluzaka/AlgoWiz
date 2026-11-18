// windows.h first (see Terminal.cpp for why).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <fstream>

#include "PluginLoader.h"

typedef int (*VersionFn)();
typedef void (*RegisterFn)(PluginRegistry*);

string PluginLoader::load(const string& path, PluginRegistry& registry) {
    {
        std::ifstream probe(path, std::ios::binary);
        if (!probe) return "The file \"" + path + "\" does not exist or cannot be read.";
    }
#ifdef _WIN32
    HMODULE lib = LoadLibraryA(path.c_str());
    if (!lib) return "The library \"" + path + "\" could not be loaded (Windows error " + to_string((long)GetLastError()) + ").";
    VersionFn version = (VersionFn)(void*)GetProcAddress(lib, "algoviz_plugin_api_version");
    RegisterFn reg = (RegisterFn)(void*)GetProcAddress(lib, "algoviz_register");
#else
    void* lib = dlopen(path.c_str(), RTLD_NOW);
    if (!lib) return string("The library could not be loaded: ") + dlerror();
    VersionFn version = (VersionFn)dlsym(lib, "algoviz_plugin_api_version");
    RegisterFn reg = (RegisterFn)dlsym(lib, "algoviz_register");
#endif
    if (!version || !reg) return "\"" + path + "\" is not an Algo Visualizer plugin (missing algoviz_register / algoviz_plugin_api_version).";
    if (version() != PLUGIN_API_VERSION)
        return "The plugin was built for plugin API version " + to_string(version()) + ", this program uses version " + to_string(PLUGIN_API_VERSION) + ".";
    size_t before = registry.count();
    try {
        reg(&registry);
    } catch (const std::exception& e) {
        return string("The plugin failed while registering: ") + e.what();
    } catch (...) {
        return "The plugin failed while registering.";
    }
    if (registry.count() == before) return "The plugin loaded but did not register anything (see the plugin log).";
    return "";
}
