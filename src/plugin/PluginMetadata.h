#ifndef PLUGINMETADATA_H
#define PLUGINMETADATA_H

#include <string>
#include <vector>

#include "../core/AlgorithmCatalog.h"
#include "../engine/Step.h"

using namespace std;

enum class PluginKind { Algorithm, Structure };

// Everything the host needs to know about a plugin without running it.
struct PluginMetadata {
    string id;                       // unique key, lowercase letters / digits / underscores
    string name;                     // shown in the menu
    string author = "unknown";
    string version = "1.0";
    string description;
    PluginKind kind = PluginKind::Algorithm;
    StructureKind renders = StructureKind::Array;   // which built-in renderer draws this plugin
    string inputSchema = "array";    // algorithms: "array" | "array+target" | "words" | "none"
    string best = "?", average = "?", worst = "?", space = "?";
    vector<string> pseudocode;
    vector<OpSpec> ops;              // structures: the operations offered in the menu

    // Empty string = valid, otherwise the reason the plugin is rejected.
    string problem() const {
        if (id.empty() || name.empty()) return "A plugin needs a non-empty id and name.";
        for (char c : id) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return "Plugin id \"" + id + "\" may only contain a-z, 0-9 and _.";
        if (kind == PluginKind::Structure && ops.empty()) return "A custom structure must declare at least one operation.";
        if (kind == PluginKind::Algorithm && inputSchema != "array" && inputSchema != "array+target" && inputSchema != "words" && inputSchema != "none")
            return "Unknown input schema \"" + inputSchema + "\".";
        return "";
    }
};

#endif
