#ifndef ALGORITHMCATALOG_H
#define ALGORITHMCATALOG_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../engine/Algorithm.h"

using namespace std;

// What the input screen has to ask for.
enum class InputKind { Array, Words, Search, Structure };

// One operation of a data-structure session, e.g. {"insert_end", "Insert at end", {"value"}}.
struct OpSpec {
    string id;
    string label;
    vector<string> prompts;   // one integer prompt per argument (Command::a, b, c)
    bool list = false;        // true: ask for a whole list of integers, stored in Command::s (space separated)
};

struct CatalogEntry {
    string id;                // stable key used by --dump and the tests
    string category;          // menu category: "Sorting", "Searching", "Trees" ...
    string group;             // optional heading inside the category ("String sorting")
    string name;              // display name
    InputKind input = InputKind::Array;
    StructureKind structure = StructureKind::Array;
    function<unique_ptr<Algorithm>()> make;
    vector<OpSpec> ops;       // only for InputKind::Structure
    bool custom = false;      // true for plugin entries (menu item 13)
};

class AlgorithmCatalog {
public:
    void add(CatalogEntry e) { entries_.push_back(std::move(e)); }
    const vector<CatalogEntry>& all() const { return entries_; }

    const CatalogEntry* find(const string& id) const {
        for (const auto& e : entries_) if (e.id == id) return &e;
        return nullptr;
    }
    vector<const CatalogEntry*> inCategory(const string& category) const {
        vector<const CatalogEntry*> r;
        for (const auto& e : entries_) if (e.category == category && !e.custom) r.push_back(&e);
        return r;
    }
    vector<const CatalogEntry*> custom() const {
        vector<const CatalogEntry*> r;
        for (const auto& e : entries_) if (e.custom) r.push_back(&e);
        return r;
    }
    void removeCustom() {
        vector<CatalogEntry> keep;
        for (auto& e : entries_) if (!e.custom) keep.push_back(std::move(e));
        entries_ = std::move(keep);
    }
private:
    vector<CatalogEntry> entries_;
};

// Shared helper used by every registerXxx() function.
template <class A>
CatalogEntry makeEntry(const string& group, InputKind in, StructureKind sk, vector<OpSpec> ops = {}) {
    CatalogEntry e;
    unique_ptr<Algorithm> probe(new A());
    e.id = probe->info().id;
    e.category = probe->info().category;
    e.name = probe->info().name;
    e.group = group;
    e.input = in;
    e.structure = sk;
    e.ops = std::move(ops);
    e.make = [] { return unique_ptr<Algorithm>(new A()); };
    return e;
}

#endif
