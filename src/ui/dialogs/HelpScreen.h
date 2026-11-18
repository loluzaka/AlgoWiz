#ifndef HELPSCREEN_H
#define HELPSCREEN_H

// Help dialog (the tabbed QDialog of the Qt plan): shortcuts, colours/roles, pointers and
// controls. The colour page is drawn with the ACTIVE theme, so it always matches the screen.

#include <string>
#include <vector>

#include "../../engine/Describe.h"
#include "../../render/primitives/Canvas.h"
#include "../../render/primitives/Shapes.h"
#include "../../render/theme/Theme.h"
#include "../Terminal.h"

using namespace std;

inline vector<string> helpTabNames() { return {"Keys", "Colors", "Pointers", "Controls"}; }

inline Canvas buildHelpPage(int tab, int W, int H, const Theme& th, const GlyphSet& g) {
    Canvas c(W, H);
    string title = "HELP  -  ";
    auto tabs = helpTabNames();
    for (size_t i = 0; i < tabs.size(); i++) title += (i == (size_t)tab ? "[" + tabs[i] + "]" : " " + tabs[i] + " ") + " ";
    c.put(0, 0, title, th.header);
    c.put(0, 1, g.repeat(g.h, W), th.border);
    int y = 3;
    auto line = [&](const string& a, const string& b, const Style& st) { c.put(2, y, a, st); c.put(18, y, b, th.normal); y++; };
    switch (tab) {
    case 0:
        for (auto kv : vector<pair<string, string>>{{"P", "Play"}, {"Space", "Pause / resume"}, {"N or Right", "Next step"}, {"B or Left", "Previous step (pointer rewind)"},
                {"R", "Reset to the initial state"}, {"J", "Jump to a step number"}, {"S", "Change speed (Slow, Normal, Fast, Turbo)"}, {"F", "Fast-forward x4 on/off"},
                {"K", "Skip animation: play straight to the end"}, {"I", "Enter new input data"}, {"C", "Show / hide the code view"}, {"V", "Change view (graphs: matrix / drawing / list)"},
                {"T", "Next theme"}, {"U", "Unicode / ASCII drawing characters"}, {"E", "Export summary_report.md"},
                {"Up / Down", "Scroll the picture vertically"}, {"[ and ]", "Scroll the picture horizontally (wide trees)"}, {"H", "This help"}, {"Q or Esc", "Back to the previous menu"}})
            line(kv.first, kv.second, th.accent);
        break;
    case 1:
        c.put(2, y++, "Each highlight has a meaning. In monochrome / high-contrast / colour-blind themes a bracketed tag is also printed.", th.normal);
        y++;
        for (Role r : {Role::Compared, Role::Moved, Role::Sorted, Role::Pivot, Role::Found, Role::NotFound, Role::Current, Role::Active, Role::Candidate, Role::Visited,
                       Role::Queued, Role::Frontier, Role::Path, Role::Collision, Role::Eliminated, Role::Range, Role::GroupA, Role::GroupB, Role::Error}) {
            if (y >= H - 1) break;
            RoleStyle rs = th.role(r);
            string badge = th.badge(r);
            if (badge.empty()) { Theme t2 = th; t2.badges = true; badge = t2.badge(r); }
            c.put(2, y, "  " + badge + " " + roleName(r) + "  ", rs.style);
            string meaning;
            switch (r) {
            case Role::Compared: meaning = "being compared right now"; break;       case Role::Moved: meaning = "just swapped, shifted or written"; break;
            case Role::Sorted: meaning = "in its final position"; break;           case Role::Pivot: meaning = "pivot of a partition"; break;
            case Role::Found: meaning = "the search result"; break;                case Role::NotFound: meaning = "the search failed"; break;
            case Role::Current: meaning = "the single cursor position"; break;    case Role::Active: meaning = "about to be used"; break;
            case Role::Candidate: meaning = "candidate (minimum, node to delete ...)"; break; case Role::Visited: meaning = "already processed"; break;
            case Role::Queued: meaning = "waiting in a queue / stack"; break;     case Role::Frontier: meaning = "discovered, not yet visited"; break;
            case Role::Path: meaning = "on the path being followed"; break;       case Role::Collision: meaning = "hash collision"; break;
            case Role::Eliminated: meaning = "discarded from the search range"; break; case Role::Range: meaning = "inside the live range / partition"; break;
            case Role::GroupA: meaning = "left run / first group"; break;        case Role::GroupB: meaning = "right run / second group"; break;
            default: meaning = "error or invalid state"; break;
            }
            c.put(34, y, meaning, th.normal);
            y++;
        }
        break;
    case 2:
        for (auto kv : vector<pair<string, string>>{{"^i  ^j  ^mid", "indexes used by sorting and searching algorithms (arrow under the cell)"},
                {"^lo  ^hi", "ends of the live search range"}, {"^min  ^key  ^pivot", "selection / insertion / quick sort helpers"},
                {"HEAD  TAIL", "first and last node of a linked list"}, {"^cur  ^new  ^del", "node being walked, created or deleted"},
                {"TOP", "last pushed element of a stack"}, {"FRONT  REAR", "dequeue end and enqueue end of a queue"},
                {"^root", "root of a tree"}, {"bf=+1", "AVL balance factor = height(left) - height(right)"},
                {"[3]", "index of a heap element (parent = (i-1)/2)"}, {"NULL", "end of a list: no next node"}, {"DEL", "deleted hash slot (tombstone)"}})
            line(kv.first, kv.second, th.accent);
        break;
    default:
        c.put(2, y++, "Menus: type the number and press Enter. 0 goes back. Invalid input never crashes - you are simply asked again.", th.normal);
        c.put(2, y++, "Typing  q  at any prompt cancels it.", th.normal);
        y++;
        c.put(2, y++, "Sorting / searching: choose manual, random or preset data. Binary and interpolation search need sorted data.", th.normal);
        c.put(2, y++, "Data structures: pick an operation, enter its values; the structure keeps its state between operations.", th.normal);
        c.put(2, y++, "Race Mode: two algorithms on the same input, side by side. Quiz Mode: predict the next step and collect points.", th.normal);
        c.put(2, y++, "Custom: algorithms and structures written as plugins appear in menu 13 and behave like built-in ones.", th.normal);
        y++;
        c.put(2, y++, "Display: if the characters look wrong press U for ASCII; if colours look wrong press T until the Monochrome theme.", th.normal);
        c.put(2, y++, "Command line:  algoviz --ascii  --mono  --list  --dump <id>   (see Documentation/Project_Wide/Build_Guide.md)", th.dim);
        break;
    }
    c.put(0, H - 1, "Left/Right: change tab    any other key: close", th.dim);
    return c;
}

inline void showHelp(const Theme& th, const GlyphSet& g) {
    int tab = 0;
    for (;;) {
        int W = Terminal::width(), H = Terminal::height();
        Canvas page = buildHelpPage(tab, W, H, th, g);
        Terminal::clear();
        Terminal::write(th.color ? page.toAnsi(H) : page.toPlain(H, false));
        int k = Terminal::waitKey();
        if (k == K_RIGHT) tab = (tab + 1) % 4;
        else if (k == K_LEFT) tab = (tab + 3) % 4;
        else break;
    }
}

#endif
