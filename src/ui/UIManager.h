#ifndef UIMANAGER_H
#define UIMANAGER_H

// Navigation: home menu (spec section 3), category menus, input dialogs and data-structure
// sessions, and Race Mode. It owns no algorithm logic: it
// asks the dialogs for input, calls generate(), and hands the Timeline to a screen.

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../core/BuiltinCatalog.h"
#include "../engine/quiz/QuizController.h"
#include "../engine/race/RaceController.h"
#include "InputManager.h"
#include "Settings.h"
#include "Terminal.h"
#include "VisualizerScreen.h"
#include "dialogs/HelpScreen.h"
#include "dialogs/InputDialogs.h"
#include "race/RaceScreen.h"

using namespace std;

inline const vector<string>& homeMenuItems() {
    static const vector<string> items = {"Arrays", "Searching", "Sorting", "Linked Lists", "Stack", "Queue", "Hashing", "Trees", "Heaps", "Graphs",
                                         "Race Mode (Compare Two Algorithms)"};
    return items;
}

class UIManager {
public:
    UIManager(Settings& s, AlgorithmCatalog& catalog) : settings_(s), catalog_(catalog) {}

    void run() {
        for (;;) {
            drawHome();
            string line = InputManager::trim(Terminal::readLine(" Enter option: "));
            if (line.empty()) continue;
            char c = (char)tolower((unsigned char)line[0]);
            if (line.size() == 1 && c == 'h') { showHelp(Theme::make(settings_.theme), glyphsFor(settings_)); continue; }
            if (line.size() == 1 && c == 't') { settings_.theme = Theme::next(settings_.theme); settings_.save(); continue; }
            if (line.size() == 1 && c == 'u') { settings_.unicode = !settings_.unicode; settings_.save(); continue; }
            int n;
            string err = InputManager::parseInt(line, 0, (int)homeMenuItems().size(), n);
            if (!err.empty()) { notice("Invalid choice. " + err); continue; }
            if (n == 0) return;
            if (n <= 10) categoryMenu(menuCategories()[n - 1]);
            else raceMenu();
        }
    }

private:
    // ------------------------------------------------------------------ helpers
    void notice(const string& msg) { Terminal::write("\n  " + msg + "\n"); Terminal::readLine("  Press Enter to continue... "); }
    Theme theme() const { return Theme::make(settings_.theme); }
    void show(const Canvas& c, int rows) { Theme th = theme(); Terminal::write(frameText(c, th, rows) + "\n"); }

    void drawHome() {
        Theme th = theme();
        GlyphSet g = glyphsFor(settings_);
        const vector<string>& items = homeMenuItems();
        int inner = 62;
        Canvas c(inner + 2, (int)items.size() + 8);
        auto edge = [&](int y, const string& l, const string& r) { c.put(0, y, l + g.repeat(g.h, inner) + r, th.border); };
        auto row = [&](int y, const string& text, const Style& st) { c.put(0, y, g.v, th.border); c.put(inner + 1, y, g.v, th.border); c.putCentered(1, inner, y, text, st); };
        auto leftRow = [&](int y, const string& text, const Style& st) { c.put(0, y, g.v, th.border); c.put(inner + 1, y, g.v, th.border); c.put(3, y, text, st); };
        edge(0, g.tl, g.tr);
        row(1, "ALGO VISUALIZER", th.header);
        row(2, "DATA STRUCTURES & ALGORITHMS", th.normal);
        edge(3, g.tl, g.tr);
        for (size_t i = 0; i < items.size(); i++) leftRow(4 + (int)i, (i + 1 < 10 ? " " : "") + to_string(i + 1) + ". " + items[i], th.normal);
        int y = 4 + (int)items.size();
        leftRow(y, "", th.normal);
        leftRow(y + 1, " 0. Exit", th.normal);
        edge(y + 2, g.bl, g.br);
        c.put(1, y + 3, "Enter option: ", th.accent);
        Terminal::clear();
        Terminal::write(frameText(c, th, y + 4) + "\n");
        // the prompt text is part of the frame above; the cursor sits after it
        Terminal::write("\x1b[1A\x1b[" + to_string(16) + "C\x1b[?25h");
    }

    // ----------------------------------------------------------- category menu
    void categoryMenu(const string& category) {
        for (;;) {
            auto entries = catalog_.inCategory(category);
            Terminal::clear();
            Terminal::write("== " + category + " ==\n\n");
            string lastGroup = "\x01";
            int idx = 1;
            for (auto* e : entries) {
                if (e->group != lastGroup && !e->group.empty()) Terminal::write("  -- " + e->group + " --\n");
                lastGroup = e->group;
                Terminal::write("   " + string(idx < 10 ? " " : "") + to_string(idx) + ". " + e->name + "\n");
                idx++;
            }
            Terminal::write("\n    0. Back\n");
            int n;
            if (!InputManager::askInt("Choose", 0, (int)entries.size(), n) || n == 0) return;
            runEntry(*entries[n - 1]);
        }
    }

    void runEntry(const CatalogEntry& e) {
        if (e.input == InputKind::Structure) { structureSession(e); return; }
        Input in;
        for (;;) {
            if (!Dialogs::askEntryInput(e, in)) return;
            unique_ptr<Algorithm> algo = e.make();
            Timeline tl = generate(*algo, in);
            VisualizerScreen screen(settings_, catalog_, e, algo->info());
            screen.load(std::move(tl));
            if (screen.run() == VisualizerScreen::Result::Back) return;
        }
    }

    // ----------------------------------------------------------- structure session
    void structureSession(const CatalogEntry& e) {
        Input in;
        if (!Dialogs::askStructureSetup(e, in)) return;
        unique_ptr<Algorithm> algo = e.make();
        for (;;) {
            in.visibleFrom = in.commands.size();                       // everything so far is history
            Timeline state = generate(*algo, in);
            if (!state.error.empty()) { notice(state.error); return; }
            Terminal::clear();
            previewState(e, algo->info(), state);
            Terminal::write("\n  Operations on " + e.name + " (" + to_string(in.commands.size()) + " performed so far):\n");
            for (size_t i = 0; i < e.ops.size(); i++) Terminal::write("   " + string(i + 1 < 10 ? " " : "") + to_string(i + 1) + ". " + e.ops[i].label + "\n");
            Terminal::write("    u. Undo the last operation     r. Start over with new set-up     0. Back\n");
            string line = InputManager::trim(Terminal::readLine("  Choose: "));
            if (line == "0" || line == "q" || line == "Q") return;
            if (line == "u" || line == "U") {
                if (in.commands.empty()) notice("Nothing to undo yet.");
                else in.commands.pop_back();
                continue;
            }
            if (line == "r" || line == "R") { if (!Dialogs::askStructureSetup(e, in)) return; continue; }
            int n;
            string err = InputManager::parseInt(line, 1, (int)e.ops.size(), n);
            if (!err.empty()) { notice("Invalid choice. " + err); continue; }
            Command cmd;
            if (!Dialogs::askOperation(e.ops[n - 1], cmd)) continue;
            in.commands.push_back(cmd);
            in.visibleFrom = in.commands.size() - 1;
            Timeline tl = generate(*algo, in);
            if (!tl.error.empty()) {                                    // invalid operation: explain, keep the structure unchanged
                in.commands.pop_back();
                notice(tl.error);
                continue;
            }
            VisualizerScreen screen(settings_, catalog_, e, algo->info());
            screen.load(std::move(tl));
            auto r = screen.run();
            if (r == VisualizerScreen::Result::NewInput) { if (!Dialogs::askStructureSetup(e, in)) return; }
        }
    }

    void previewState(const CatalogEntry& e, const AlgorithmInfo& info, const Timeline& state) {
        (void)info;
        Theme th = theme();
        GlyphSet g = glyphsFor(settings_);
        int W = Terminal::width() - 1;
        ExecutionEngine eng;
        eng.load(state);
        VizPane pane;
        Canvas c(W, 18);
        c.put(0, 0, e.name + " - current state", th.header);
        pane.draw(c, 0, 1, W, 17, eng, th, g);
        int rows = max(4, c.usedHeight());
        Terminal::write(frameText(c, th, rows) + "\n");
    }

    // ------------------------------------------------------------------- race
    void raceMenu() {
        Terminal::clear();
        Terminal::write("== Race Mode: compare two algorithms on the same input ==\n\n");
        int grp = InputManager::askChoice("What do you want to race?", {"Numeric sorting algorithms", "String sorting algorithms", "Searching algorithms"});
        if (grp < 0) return;
        vector<const CatalogEntry*> pool;
        for (const CatalogEntry& e : catalog_.all()) {
            bool numeric = e.category == "Sorting" && e.input == InputKind::Array;
            bool strings = e.category == "Sorting" && e.input == InputKind::Words;
            bool search = e.category == "Searching";
            if ((grp == 0 && numeric) || (grp == 1 && strings) || (grp == 2 && search)) pool.push_back(&e);
        }
        if (pool.size() < 2) { notice("Fewer than two algorithms are available in this group."); return; }
        vector<string> names;
        for (auto* e : pool) names.push_back(e->name);
        int a = InputManager::askChoice("\nFirst algorithm (left pane):", names);
        if (a < 0) return;
        int b = InputManager::askChoice("\nSecond algorithm (right pane):", names);
        if (b < 0) return;
        if (a == b) { notice("Please choose two different algorithms."); return; }
        Input in;
        const CatalogEntry& ea = *pool[a];
        const CatalogEntry& eb = *pool[b];
        if (!Dialogs::askEntryInput(ea, in)) return;
        bool needsSorted = ea.id == "binary_search" || ea.id == "interpolation_search" || eb.id == "binary_search" || eb.id == "interpolation_search";
        if (needsSorted && !is_sorted(in.values.begin(), in.values.end())) {
            sort(in.values.begin(), in.values.end());
            Terminal::write("  (One of the searches needs sorted data, so the array was sorted for both.)\n");
        }
        auto algoA = ea.make(), algoB = eb.make();
        Timeline ta = generate(*algoA, in), tb = generate(*algoB, in);
        if (!ta.error.empty()) { notice(ea.name + ": " + ta.error); return; }
        if (!tb.error.empty()) { notice(eb.name + ": " + tb.error); return; }
        RaceController race;
        race.load(std::move(ta), std::move(tb), ea.name, eb.name);
        RaceScreen screen(settings_, race, in, algoA->info(), algoB->info());
        screen.run();
    }

    Settings& settings_;
    AlgorithmCatalog& catalog_;
};

#endif
