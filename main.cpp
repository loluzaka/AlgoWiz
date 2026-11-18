// ALGO VISUALIZER - entry point.
//   algoviz                 interactive terminal application
//   algoviz --list          list every algorithm id
//   algoviz --dump <id> ... print one frame / the step list / a report without a terminal
// Options:  --ascii  --unicode  --mono  --theme N  --plugin <library>  --help
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "src/core/BuiltinCatalog.h"
#include "src/plugin/PluginLoader.h"
#include "src/plugin/examples/ExamplePlugins.h"
#include "src/ui/Headless.h"
#include "src/ui/Settings.h"
#include "src/ui/Terminal.h"
#include "src/ui/UIManager.h"

static void printUsage() {
    std::cout <<
        "ALGO VISUALIZER - data structures and algorithms, step by step\n\n"
        "  algoviz                      start the interactive application\n"
        "  algoviz --list               list all algorithm ids\n"
        "  algoviz --dump <id> [opts]   print a frame without a terminal, e.g.\n"
        "      algoviz --dump bubble_sort --values 5,3,1,4 --step 3\n"
        "      algoviz --dump bst --ops \"insert 35; delete 50\" --end\n"
        "      algoviz --dump avl --ops \"insert 10; insert 20; insert 30\" --steps\n"
        "      dump options: --values --target --words --ops --step N --end --steps --report\n"
        "                    --theme 0-4 --width W --height H --descending --capacity N --table N\n"
        "                    --directed --ansi --code --mode 0-3 --nocase\n"
        "  Options for the interactive mode: --ascii --unicode --mono --theme N --plugin <library>\n";
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    AlgorithmCatalog catalog = buildBuiltinCatalog();
    PluginRegistry plugins;
    registerExamplePlugins(plugins);                // compiled-in plugins (menu item 13)
    plugins.addToCatalog(catalog);

    Settings settings;
    settings.load();
    settings.unicode = settings.unicode && Terminal::supportsUnicode();

    // ---- headless commands --------------------------------------------------------------
    if (!args.empty()) {
        if (args[0] == "--help" || args[0] == "-h") { printUsage(); return 0; }
        if (args[0] == "--list") { std::cout << Headless::list(catalog); return 0; }
        if (args[0] == "--dump") {
            bool ok;
            std::string text = Headless::dump(catalog, std::vector<std::string>(args.begin() + 1, args.end()), ok);
            std::cout << text;
            return ok ? 0 : 2;
        }
    }

    // ---- interactive options ------------------------------------------------------------
    std::vector<std::string> libraries;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--ascii") settings.unicode = false;
        else if (args[i] == "--unicode") settings.unicode = true;
        else if (args[i] == "--mono") settings.theme = ThemeId::Mono;
        else if (args[i] == "--theme" && i + 1 < args.size()) settings.theme = static_cast<ThemeId>(std::max(0, std::min(4, std::atoi(args[++i].c_str()))));
        else if (args[i] == "--plugin" && i + 1 < args.size()) libraries.push_back(args[++i]);
        else { std::cerr << "Unknown option: " << args[i] << "\n"; printUsage(); return 2; }
    }
    for (const std::string& lib : libraries) {
        std::string err = PluginLoader::load(lib, plugins);
        if (!err.empty()) std::cerr << "Plugin not loaded: " << err << "\n";
    }
    plugins.addToCatalog(catalog);

    if (!Terminal::isInteractive() && !std::getenv("ALGOVIZ_FORCE_INTERACTIVE")) {
        std::cerr << "This program needs an interactive terminal. Use --list or --dump <id> for non-interactive output.\n";
        return 1;
    }
    Terminal::init();
    UIManager ui(settings, catalog);
    ui.run();
    Terminal::clear();
    Terminal::shutdown();
    return 0;
}
