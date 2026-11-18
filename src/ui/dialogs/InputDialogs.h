#ifndef INPUTDIALOGS_H
#define INPUTDIALOGS_H

// The input dialogs of spec section 26: manual / random / preset data, search targets, word
// lists with a sorting criterion, structure set-up and single operations. All numeric input goes
// through InputManager, so bad input produces a message and a new prompt, never a crash.

#include <algorithm>
#include <climits>
#include <string>
#include <vector>

#include "../../core/AlgorithmCatalog.h"
#include "../InputManager.h"
#include "../Terminal.h"

using namespace std;

namespace Dialogs {

struct PresetArray { string name; vector<int> values; };
inline const vector<PresetArray>& presetArrays() {
    static const vector<PresetArray> p = {
        {"Specification example", {8, 3, 5, 1, 9, 2}}, {"Reverse sorted", {9, 7, 5, 3, 1}}, {"Already sorted", {1, 2, 3, 4, 5, 6}},
        {"With duplicates", {4, 2, 4, 1, 2, 4}}, {"Two elements", {7, 3}}, {"Small range (counting / radix friendly)", {29, 3, 17, 3, 25, 11, 40}}};
    return p;
}
struct PresetWords { string name; vector<string> words; };
inline const vector<PresetWords>& presetWords() {
    static const vector<PresetWords> p = {
        {"Fruits", {"banana", "apple", "mango", "cherry"}}, {"Mixed case", {"Delta", "alpha", "Charlie", "bravo", "Echo"}},
        {"Common prefixes", {"apricot", "apple", "ape", "application", "apply"}}};
    return p;
}

inline void heading(const string& t) { Terminal::clear(); Terminal::write("== " + t + " ==\n\n"); }

inline bool yesNo(const string& question, bool defaultYes) {
    string a = InputManager::trim(Terminal::readLine(question + (defaultYes ? " [Y/n]: " : " [y/N]: ")));
    if (a.empty()) return defaultYes;
    return a[0] == 'y' || a[0] == 'Y';
}

// Empty line = no values. Returns false only if the user typed q.
inline bool askOptionalList(const string& prompt, int maxCount, int lo, int hi, vector<int>& out) {
    for (;;) {
        string line = InputManager::trim(Terminal::readLine(prompt + " (Enter = none, q = cancel): "));
        if (line == "q" || line == "Q") return false;
        if (line.empty()) { out.clear(); return true; }
        string err = InputManager::parseIntList(line, 1, maxCount, lo, hi, out);
        if (err.empty()) return true;
        Terminal::write("  ! " + err + "\n");
    }
}

inline bool askArray(Input& in, bool needSorted, int maxCount = 20) {
    heading("Array input");
    int src = InputManager::askChoice("Where should the values come from?", {"Type the values myself", "Random values", "Preset sample data"});
    if (src < 0) return false;
    if (src == 0) {
        if (!InputManager::askIntList("Enter 1-" + to_string(maxCount) + " whole numbers (-999..999) separated by spaces", 1, maxCount, -999, 999, in.values)) return false;
    } else if (src == 1) {
        int n, lo, hi;
        if (!InputManager::askInt("How many values", 1, maxCount, n, 8)) return false;
        if (!InputManager::askInt("Smallest value", -999, 999, lo, 1)) return false;
        if (!InputManager::askInt("Largest value", lo, 999, hi, max(lo, 99))) return false;
        in.values = InputManager::randomValues(n, lo, hi);
    } else {
        vector<string> names;
        for (const auto& p : presetArrays()) names.push_back(p.name + "   " + [&] { string s; for (int v : p.values) s += to_string(v) + " "; return s; }());
        int c = InputManager::askChoice("Choose a preset", names);
        if (c < 0) return false;
        in.values = presetArrays()[c].values;
    }
    if (needSorted && !is_sorted(in.values.begin(), in.values.end())) {
        if (yesNo("This algorithm needs sorted data. Sort the array automatically?", true)) sort(in.values.begin(), in.values.end());
    }
    return true;
}

inline bool askTarget(Input& in) {
    int c = InputManager::askChoice("Search target:", {"Type a number", "A random value that IS in the array", "A random value that is NOT in the array"});
    if (c < 0) return false;
    in.hasTarget = true;
    if (c == 0) return InputManager::askInt("Target", -999, 999, in.target);
    mt19937 rng(random_device{}());
    if (c == 1 && !in.values.empty()) in.target = in.values[rng() % in.values.size()];
    else {
        int t = 1000;
        for (int tries = 0; tries < 100; tries++) { t = (int)(rng() % 200) - 50; if (find(in.values.begin(), in.values.end(), t) == in.values.end()) break; }
        in.target = t;
    }
    Terminal::write("  Target chosen: " + to_string(in.target) + "\n");
    return true;
}

inline bool askWords(Input& in) {
    heading("Word input");
    int src = InputManager::askChoice("Where should the words come from?", {"Type the words myself", "Preset word list"});
    if (src < 0) return false;
    if (src == 0) {
        if (!InputManager::askWords("Enter 1-14 words separated by spaces", 1, 14, in.words)) return false;
    } else {
        vector<string> names;
        for (const auto& p : presetWords()) names.push_back(p.name + "   " + [&] { string s; for (auto& w : p.words) s += w + " "; return s; }());
        int c = InputManager::askChoice("Choose a preset", names);
        if (c < 0) return false;
        in.words = presetWords()[c].words;
    }
    int m = InputManager::askChoice("Sorting criterion:", {"Alphabetical (A-Z)", "Reverse alphabetical (Z-A)", "Length, shortest first", "Length, longest first"});
    if (m < 0) return false;
    in.stringMode = static_cast<StringMode>(m);
    in.caseInsensitive = false;
    if (m < 2) in.caseInsensitive = yesNo("Ignore upper/lower case?", false);
    return true;
}

inline bool askDirection(Input& in) {
    int d = InputManager::askChoice("Order:", {"Ascending (small to large)", "Descending (large to small)"});
    if (d < 0) return false;
    in.descending = d == 1;
    return true;
}

// Input for algorithms that run once on a data set (sorting, searching, strings, custom algorithms).
inline bool askEntryInput(const CatalogEntry& e, Input& in) {
    in = Input();
    bool needsSorted = e.id == "binary_search" || e.id == "interpolation_search";
    switch (e.input) {
    case InputKind::Array:
        if (!askArray(in, false)) return false;
        if (e.category == "Sorting" && !askDirection(in)) return false;
        return true;
    case InputKind::Search:
        if (!askArray(in, needsSorted)) return false;
        return askTarget(in);
    case InputKind::Words:
        return askWords(in);
    case InputKind::Structure:
        break;
    }
    return true;
}

// Set-up questions for a data-structure session (initial contents, capacity, table size ...).
inline bool askStructureSetup(const CatalogEntry& e, Input& in) {
    in = Input();
    heading("Set up: " + e.name);
    switch (e.structure) {
    case StructureKind::Stack: case StructureKind::Queue:
        if (!InputManager::askInt("Capacity (number of cells)", 1, 12, in.capacity, 6)) return false;
        return askOptionalList("Initial values (up to the capacity)", in.capacity, -999, 999, in.values);
    case StructureKind::Hash:
        if (!InputManager::askInt("Table size (number of slots)", 3, 13, in.tableSize, 7)) return false;
        return askOptionalList("Initial keys", 10, -999, 999, in.values);
    case StructureKind::Heap:
        return askOptionalList("Initial values (up to 15)", 15, -999, 999, in.values);
    case StructureKind::Graph: {
        in.directed = yesNo("Directed graph (edges have a direction)?", false);
        int p = InputManager::askChoice("Starting graph:", {"Sample graph with 6 vertices (A-F)", "Empty graph", "Only vertices A..N (I add the edges)"});
        if (p < 0) return false;
        if (p == 0) { in.vertexCount = 6; in.values = {0, 1, 4, 0, 2, 2, 1, 3, 5, 2, 3, 1, 2, 4, 7, 3, 5, 3}; }
        else if (p == 2) { if (!InputManager::askInt("Number of vertices", 1, 10, in.vertexCount, 5)) return false; }
        return true;
    }
    case StructureKind::Tree:
        if (yesNo("Start with a sample tree (50 30 70 20 40 60 80)?", true)) { in.values = {50, 30, 70, 20, 40, 60, 80}; return true; }
        return askOptionalList("Initial values", 15, -999, 999, in.values);
    default:
        return askOptionalList("Initial values", 12, -999, 999, in.values);
    }
}

// One operation of a structure session. `argsLeft` prompts are asked in order (a, b, c).
inline bool askOperation(const OpSpec& op, Command& cmd) {
    cmd = Command();
    cmd.op = op.id;
    if (op.list) {
        vector<int> v;
        if (!InputManager::askIntList(op.label + ": enter the numbers separated by spaces", 1, 15, -999, 999, v)) return false;
        for (size_t i = 0; i < v.size(); i++) cmd.s += (i ? " " : "") + to_string(v[i]);
        return true;
    }
    int* slots[3] = {&cmd.a, &cmd.b, &cmd.c};
    cmd.c = 1;                                        // default edge weight
    for (size_t i = 0; i < op.prompts.size() && i < 3; i++)
        if (!InputManager::askInt(op.prompts[i], -999, 999, *slots[i])) return false;
    return true;
}

} // namespace Dialogs

#endif
