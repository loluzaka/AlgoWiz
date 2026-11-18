#ifndef HEADLESS_H
#define HEADLESS_H

// Non-interactive commands used for documentation screenshots, quick checks and the tests:
//   algoviz --list
//   algoviz --dump <id> [--values 5,3,1] [--target 3] [--words a,b,c] [--ops "insert 5; delete 3"]
//                       [--step N | --end | --steps] [--theme 0-4] [--width W] [--height H]
//                       [--descending] [--capacity N] [--table N] [--directed] [--ansi] [--code] [--report]
// Nothing here touches the terminal, so the output can be redirected to a file.

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../core/BuiltinCatalog.h"
#include "../engine/report/ReportExporter.h"
#include "VisualizerScreen.h"

using namespace std;

namespace Headless {

inline string list(const AlgorithmCatalog& cat) {
    ostringstream o;
    for (const auto& e : cat.all())
        o << e.id << "\t" << e.category << (e.group.empty() ? "" : " / " + e.group) << "\t" << e.name << "\n";
    return o.str();
}

inline vector<string> splitBy(const string& s, char sep) {
    vector<string> r;
    string cur;
    for (char c : s) { if (c == sep) { r.push_back(cur); cur.clear(); } else cur += c; }
    r.push_back(cur);
    return r;
}

// Default inputs so that `--dump <id>` always shows something sensible.
inline Input defaultInput(const CatalogEntry& e) {
    Input in;
    switch (e.input) {
    case InputKind::Array: in.values = {8, 3, 5, 1, 9, 2}; break;
    case InputKind::Search: in.values = {2, 4, 6, 8, 10, 12}; in.hasTarget = true; in.target = 8; break;
    case InputKind::Words: in.words = {"banana", "apple", "mango", "cherry"}; break;
    case InputKind::Structure:
        switch (e.structure) {
        case StructureKind::Tree: in.values = {50, 30, 70, 20, 40, 60, 80}; break;
        case StructureKind::Graph: in.vertexCount = 6; in.values = {0, 1, 4, 0, 2, 2, 1, 3, 5, 2, 3, 1, 2, 4, 7, 3, 5, 3}; break;
        case StructureKind::Stack: case StructureKind::Queue: in.capacity = 6; in.values = {10, 20, 30}; break;
        case StructureKind::Hash: in.tableSize = 7; in.values = {21, 14, 8}; break;
        case StructureKind::Heap: in.values = {5, 3, 8, 1, 9, 2}; break;
        default: in.values = {10, 20, 30}; break;
        }
        break;
    }
    return in;
}

inline vector<int> parseCsv(const string& s) {
    vector<int> v;
    for (const string& t : splitBy(s, ',')) if (!t.empty()) v.push_back(atoi(t.c_str()));
    return v;
}

// "insert 5; delete 3; add_edge 0 1 4"  ->  commands matching the entry's operation ids
inline vector<Command> parseOps(const string& s) {
    vector<Command> cmds;
    for (const string& part : splitBy(s, ';')) {
        istringstream is(part);
        Command c;
        if (!(is >> c.op)) continue;
        is >> c.a >> c.b >> c.c;
        string rest;
        getline(is, rest);
        cmds.push_back(c);
    }
    return cmds;
}

// Returns the text to print; `ok` is false for usage errors.
inline string dump(AlgorithmCatalog& cat, const vector<string>& args, bool& ok) {
    ok = true;
    if (args.empty()) { ok = false; return "Usage: algoviz --dump <id> [options]   (see --list for ids)\n"; }
    const CatalogEntry* e = cat.find(args[0]);
    if (!e) { ok = false; return "Unknown algorithm id \"" + args[0] + "\". Use --list to see all ids.\n"; }
    Input in = defaultInput(*e);
    long step = -1;
    bool toEnd = false, listSteps = false, ansi = false, showCode = false, report = false;
    int theme = 4, W = 110, H = 32;
    string opsText;
    for (size_t i = 1; i < args.size(); i++) {
        const string& a = args[i];
        auto next = [&]() -> string { return i + 1 < args.size() ? args[++i] : ""; };
        if (a == "--values") in.values = parseCsv(next());
        else if (a == "--target") { in.target = atoi(next().c_str()); in.hasTarget = true; }
        else if (a == "--words") in.words = splitBy(next(), ',');
        else if (a == "--ops") opsText = next();
        else if (a == "--step") step = atol(next().c_str());
        else if (a == "--end") toEnd = true;
        else if (a == "--steps") listSteps = true;
        else if (a == "--theme") theme = atoi(next().c_str());
        else if (a == "--width") W = atoi(next().c_str());
        else if (a == "--height") H = atoi(next().c_str());
        else if (a == "--descending") in.descending = true;
        else if (a == "--capacity") in.capacity = atoi(next().c_str());
        else if (a == "--table") in.tableSize = atoi(next().c_str());
        else if (a == "--directed") in.directed = true;
        else if (a == "--ansi") ansi = true;
        else if (a == "--code") showCode = true;
        else if (a == "--report") report = true;
        else if (a == "--nocase") in.caseInsensitive = true;
        else if (a == "--mode") in.stringMode = static_cast<StringMode>(atoi(next().c_str()) % 4);
        else { ok = false; return "Unknown option " + a + "\n"; }
    }
    if (e->input == InputKind::Structure) { in.commands = parseOps(opsText); in.visibleFrom = 0; }
    unique_ptr<Algorithm> algo = e->make();
    Timeline tl = generate(*algo, in);
    ostringstream out;
    if (listSteps) {
        out << "Algorithm: " << algo->info().name << "   steps: " << tl.steps.size() << "   input: " << tl.inputDescription << "\n";
        if (!tl.error.empty()) out << "ERROR: " << tl.error << "\n";
        for (const Step& s : tl.steps) out << (s.seq + 1) << "\t" << describeStep(s) << "\t" << s.note << "\n";
        return out.str();
    }
    if (report) {
        ReportData rd; rd.info = &algo->info(); rd.timeline = &tl;
        return ReportExporter::render(rd);
    }
    Settings::noDefaultSave() = true;
    Settings st;
    st.theme = static_cast<ThemeId>(max(0, min(4, theme)));
    st.showCode = showCode;
    st.unicode = false;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(tl);
    if (toEnd) screen.engine().toEnd();
    else if (step >= 0) screen.engine().jumpTo((size_t)step);
    Canvas f = screen.compose(W, H);
    Theme th = Theme::make(st.theme);
    out << (ansi && th.color ? f.toAnsi(H) : f.toPlain(H));
    return out.str();
}

} // namespace Headless

#endif
