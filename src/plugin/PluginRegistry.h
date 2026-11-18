#ifndef PLUGINREGISTRY_H
#define PLUGINREGISTRY_H

// Collects plugins (built-in registered classes AND classes from loaded libraries) and
// publishes them to the algorithm catalog, where they appear under menu item 13.
//
// DECISION (open question in STRUCTURE.txt: QPluginLoader vs registered classes):
// both are supported. Compiled-in plugins register through registerAlgorithm() /
// registerStructure(); external libraries (.dll / .so) export a C function that calls the same
// two functions (see PluginLoader.h). Source files are NOT compiled at run time.

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "PluginAdapters.h"

using namespace std;

class PluginRegistry {
public:
    using AlgoFactory = function<unique_ptr<CustomAlgorithm>()>;
    using StructFactory = function<unique_ptr<CustomStructure>()>;

    // Both return "" on success or the reason the plugin was refused. They never throw.
    string registerAlgorithm(AlgoFactory f) {
        PluginMetadata m;
        try { unique_ptr<CustomAlgorithm> probe = f(); if (!probe) return refuse("factory returned nothing"); m = probe->metadata(); }
        catch (const exception& e) { return refuse(string("plugin constructor threw: ") + e.what()); }
        catch (...) { return refuse("plugin constructor threw"); }
        m.kind = PluginKind::Algorithm;
        string bad = check(m);
        if (!bad.empty()) return refuse(bad);
        entries_.push_back({m, f, nullptr});
        log_.push_back("Registered algorithm plugin: " + m.name);
        return "";
    }
    string registerStructure(StructFactory f) {
        PluginMetadata m;
        try { unique_ptr<CustomStructure> probe = f(); if (!probe) return refuse("factory returned nothing"); m = probe->metadata(); }
        catch (const exception& e) { return refuse(string("plugin constructor threw: ") + e.what()); }
        catch (...) { return refuse("plugin constructor threw"); }
        m.kind = PluginKind::Structure;
        string bad = check(m);
        if (!bad.empty()) return refuse(bad);
        entries_.push_back({m, nullptr, f});
        log_.push_back("Registered structure plugin: " + m.name);
        return "";
    }

    size_t count() const { return entries_.size(); }
    vector<PluginMetadata> list() const { vector<PluginMetadata> r; for (const auto& e : entries_) r.push_back(e.meta); return r; }
    const vector<string>& log() const { return log_; }

    // Replaces the catalog's custom entries with this registry's plugins.
    void addToCatalog(AlgorithmCatalog& cat) const {
        cat.removeCustom();
        for (const Entry& e : entries_) {
            if (cat.find(e.meta.id)) continue;                       // never shadow a built-in id
            CatalogEntry ce;
            ce.id = e.meta.id; ce.category = "Custom"; ce.name = e.meta.name; ce.custom = true;
            ce.group = e.meta.kind == PluginKind::Structure ? "Custom data structures" : "Custom algorithms";
            ce.structure = e.meta.renders;
            if (e.meta.kind == PluginKind::Structure) {
                ce.input = InputKind::Structure; ce.ops = e.meta.ops;
                StructFactory f = e.sf;
                ce.make = [f] { return unique_ptr<Algorithm>(new CustomStructureAdapter(f())); };
            } else {
                const string& s = e.meta.inputSchema;
                ce.input = s == "words" ? InputKind::Words : s == "array+target" ? InputKind::Search : InputKind::Array;
                AlgoFactory f = e.af;
                ce.make = [f] { return unique_ptr<Algorithm>(new CustomAlgorithmAdapter(f())); };
            }
            cat.add(ce);
        }
    }

private:
    struct Entry { PluginMetadata meta; AlgoFactory af; StructFactory sf; };
    string check(const PluginMetadata& m) const {
        string p = m.problem();
        if (!p.empty()) return p;
        for (const Entry& e : entries_) if (e.meta.id == m.id) return "a plugin with id \"" + m.id + "\" is already registered";
        return "";
    }
    string refuse(const string& why) { log_.push_back("Refused plugin: " + why); return why; }
    vector<Entry> entries_;
    vector<string> log_;
};

#endif
