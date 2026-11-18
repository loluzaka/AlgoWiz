// A plugin shipped as a separate shared library: build with  `make plugin-dll`, then use
// menu item 13 -> "Load a plugin library" and enter  build/example_plugin.dll  (or .so).
// It shows the two exported functions every plugin library needs (see PluginLoader.h).

#include "ExamplePlugins.h"
#include "../PluginLoader.h"

#ifdef _WIN32
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

PLUGIN_EXPORT int algoviz_plugin_api_version() { return PLUGIN_API_VERSION; }

PLUGIN_EXPORT void algoviz_register(PluginRegistry* registry) {
    registry->registerAlgorithm([] { return unique_ptr<CustomAlgorithm>(new CocktailSortPlugin()); });
}
