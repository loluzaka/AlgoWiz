#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

// Loads a plugin from a shared library (.dll on Windows, .so elsewhere).
//
// A plugin library must export two C functions:
//     extern "C" int  algoviz_plugin_api_version();            // must return PLUGIN_API_VERSION
//     extern "C" void algoviz_register(PluginRegistry* reg);   // calls reg->registerAlgorithm / registerStructure
//
// LIMITATION: the library and the application must be built with the SAME compiler and C++
// runtime, because C++ objects (std::string, std::vector) cross the boundary. Loaded libraries
// are never unloaded while the program runs, so their code stays valid.

#include <string>

#include "PluginRegistry.h"

using namespace std;

#define PLUGIN_API_VERSION 1

class PluginLoader {
public:
    // Returns "" on success, otherwise a message the UI can show.
    static string load(const string& path, PluginRegistry& registry);
};

#endif
