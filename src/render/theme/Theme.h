#ifndef THEME_H
#define THEME_H

// Maps a semantic Role to a visual style (colour / emphasis) and a text badge.
// Renderers NEVER hard-code colours or escape codes: they call theme.role(Role::Compared).
//
// Themes: Dark, Light, HighContrast, ColorBlind (Okabe-Ito palette) and Mono (no colour at all).
// Non-colour cues: with `badges` on, every non-default role also gets a bracketed tag such as
// [CMP] [SWP] [OK] [VIS] [PIV] [!], so meaning survives monochrome terminals and colour blindness.

#include <string>

#include "../../engine/Role.h"
#include "../primitives/Canvas.h"

using namespace std;

enum class ThemeId { Dark = 0, Light = 1, HighContrast = 2, ColorBlind = 3, Mono = 4 };

struct RoleStyle {
    Style style;
    string badge;      // "" for Role::Default
};

class Theme {
public:
    ThemeId id = ThemeId::Dark;
    bool color = true;        // false: toAnsi() output has no escape codes (Mono)
    bool badges = false;      // draw [CMP]-style tags next to highlighted entities
    Style normal;             // ordinary text
    Style dim;                // index numbers, empty slots, tree edges
    Style accent;             // structural labels: HEAD TAIL TOP FRONT REAR root i j mid
    Style border;             // box-drawing characters (kept default so panels never overpower the data)
    Style header;             // title bars
    Style error;              // error messages

    static Theme make(ThemeId id);
    static string name(ThemeId id) {
        switch (id) {
        case ThemeId::Dark: return "Dark";
        case ThemeId::Light: return "Light";
        case ThemeId::HighContrast: return "High contrast";
        case ThemeId::ColorBlind: return "Color-blind safe";
        default: return "Monochrome";
        }
    }
    static ThemeId next(ThemeId id) { return static_cast<ThemeId>((static_cast<int>(id) + 1) % 5); }

    RoleStyle role(Role r) const {
        RoleStyle rs = table_[static_cast<int>(r)];
        if (!badges) rs.badge.clear();
        return rs;
    }
    string badge(Role r) const { return role(r).badge; }

private:
    RoleStyle table_[20];
    void set(Role r, const string& badge, Style s) { table_[static_cast<int>(r)] = {s, badge}; }
    static Style fg(int c, bool bold = false, bool dim = false, bool underline = false) {
        Style s; s.fg = c; s.bold = bold; s.dim = dim; s.underline = underline; return s;
    }
    static Style bg(int back, int front, bool bold = false) {
        Style s; s.bg = back; s.fg = front; s.bold = bold; return s;
    }
};

inline Theme Theme::make(ThemeId id) {
    Theme t;
    t.id = id;
    switch (id) {
    case ThemeId::Dark:
        t.dim = fg(8); t.accent = fg(14, true); t.header = fg(15, true); t.error = fg(9, true);
        t.set(Role::Compared, "[CMP]", fg(11));            t.set(Role::Moved, "[SWP]", fg(9));
        t.set(Role::Sorted, "[OK]", fg(10));               t.set(Role::Pivot, "[PIV]", bg(5, 15, true));
        t.set(Role::Found, "[FND]", bg(2, 0, true));       t.set(Role::Active, "[ACT]", fg(13));
        t.set(Role::Current, "[CUR]", fg(13, true));       t.set(Role::Candidate, "[CAN]", fg(6, true));
        t.set(Role::Visited, "[VIS]", fg(4, false, true)); t.set(Role::Queued, "[Q]", fg(14));
        t.set(Role::Frontier, "[FRN]", fg(14));            t.set(Role::Path, "[PTH]", fg(15, true, false, true));
        t.set(Role::Error, "[!]", fg(9, true));            t.set(Role::Collision, "[COL]", bg(1, 15, true));
        t.set(Role::Eliminated, "[ELM]", fg(8, false, true)); t.set(Role::Range, "[RNG]", fg(15, false, false, true));
        t.set(Role::NotFound, "[NF]", bg(1, 15, true));    t.set(Role::GroupA, "[A]", fg(12));
        t.set(Role::GroupB, "[B]", fg(13));
        break;
    case ThemeId::Light:
        t.dim = fg(8); t.accent = fg(4, true); t.header = fg(0, true); t.error = fg(1, true);
        t.set(Role::Compared, "[CMP]", fg(130, true));     t.set(Role::Moved, "[SWP]", fg(1, true));
        t.set(Role::Sorted, "[OK]", fg(2, true));          t.set(Role::Pivot, "[PIV]", bg(5, 15, true));
        t.set(Role::Found, "[FND]", bg(2, 15, true));      t.set(Role::Active, "[ACT]", fg(5));
        t.set(Role::Current, "[CUR]", fg(5, true));        t.set(Role::Candidate, "[CAN]", fg(6, true));
        t.set(Role::Visited, "[VIS]", fg(4));              t.set(Role::Queued, "[Q]", fg(6));
        t.set(Role::Frontier, "[FRN]", fg(6, true));       t.set(Role::Path, "[PTH]", fg(0, true, false, true));
        t.set(Role::Error, "[!]", fg(1, true));            t.set(Role::Collision, "[COL]", bg(1, 15, true));
        t.set(Role::Eliminated, "[ELM]", fg(8, false, true)); t.set(Role::Range, "[RNG]", fg(0, false, false, true));
        t.set(Role::NotFound, "[NF]", bg(1, 15, true));    t.set(Role::GroupA, "[A]", fg(4, true));
        t.set(Role::GroupB, "[B]", fg(5, true));
        break;
    case ThemeId::HighContrast:
        t.badges = true;
        t.dim = fg(7); t.accent = fg(11, true); t.header = fg(15, true); t.error = bg(9, 0, true);
        t.set(Role::Compared, "[CMP]", bg(11, 0, true));   t.set(Role::Moved, "[SWP]", bg(9, 0, true));
        t.set(Role::Sorted, "[OK]", bg(10, 0, true));      t.set(Role::Pivot, "[PIV]", bg(13, 0, true));
        t.set(Role::Found, "[FND]", bg(10, 0, true));      t.set(Role::Active, "[ACT]", bg(14, 0, true));
        t.set(Role::Current, "[CUR]", bg(15, 0, true));    t.set(Role::Candidate, "[CAN]", bg(14, 0, true));
        t.set(Role::Visited, "[VIS]", fg(12, true));       t.set(Role::Queued, "[Q]", fg(14, true));
        t.set(Role::Frontier, "[FRN]", fg(14, true));      t.set(Role::Path, "[PTH]", fg(15, true, false, true));
        t.set(Role::Error, "[!]", bg(9, 0, true));         t.set(Role::Collision, "[COL]", bg(9, 15, true));
        t.set(Role::Eliminated, "[ELM]", fg(7, false, true)); t.set(Role::Range, "[RNG]", fg(15, true, false, true));
        t.set(Role::NotFound, "[NF]", bg(9, 0, true));     t.set(Role::GroupA, "[A]", fg(12, true));
        t.set(Role::GroupB, "[B]", fg(13, true));
        break;
    case ThemeId::ColorBlind:      // Okabe-Ito palette: distinguishable with protan / deutan / tritan vision
        t.badges = true;
        t.dim = fg(8); t.accent = fg(117, true); t.header = fg(15, true); t.error = fg(202, true, false, true);
        t.set(Role::Compared, "[CMP]", fg(226, true));     t.set(Role::Moved, "[SWP]", fg(202, true));
        t.set(Role::Sorted, "[OK]", fg(36, true));         t.set(Role::Pivot, "[PIV]", bg(170, 15, true));
        t.set(Role::Found, "[FND]", bg(36, 0, true));      t.set(Role::Active, "[ACT]", fg(170));
        t.set(Role::Current, "[CUR]", fg(170, true));      t.set(Role::Candidate, "[CAN]", fg(117, true));
        t.set(Role::Visited, "[VIS]", fg(33, false, true)); t.set(Role::Queued, "[Q]", fg(117));
        t.set(Role::Frontier, "[FRN]", fg(117, true));     t.set(Role::Path, "[PTH]", fg(15, true, false, true));
        t.set(Role::Error, "[!]", fg(202, true, false, true)); t.set(Role::Collision, "[COL]", bg(202, 0, true));
        t.set(Role::Eliminated, "[ELM]", fg(8, false, true)); t.set(Role::Range, "[RNG]", fg(15, false, false, true));
        t.set(Role::NotFound, "[NF]", bg(202, 0, true));   t.set(Role::GroupA, "[A]", fg(33, true));
        t.set(Role::GroupB, "[B]", fg(208, true));
        break;
    case ThemeId::Mono:
        t.color = false; t.badges = true;
        t.set(Role::Compared, "[CMP]", Style());   t.set(Role::Moved, "[SWP]", Style());
        t.set(Role::Sorted, "[OK]", Style());      t.set(Role::Pivot, "[PIV]", Style());
        t.set(Role::Found, "[FND]", Style());      t.set(Role::Active, "[ACT]", Style());
        t.set(Role::Current, "[CUR]", Style());    t.set(Role::Candidate, "[CAN]", Style());
        t.set(Role::Visited, "[VIS]", Style());    t.set(Role::Queued, "[Q]", Style());
        t.set(Role::Frontier, "[FRN]", Style());   t.set(Role::Path, "[PTH]", Style());
        t.set(Role::Error, "[!]", Style());        t.set(Role::Collision, "[COL]", Style());
        t.set(Role::Eliminated, "[ELM]", Style()); t.set(Role::Range, "[RNG]", Style());
        t.set(Role::NotFound, "[NF]", Style());    t.set(Role::GroupA, "[A]", Style());
        t.set(Role::GroupB, "[B]", Style());
        break;
    }
    return t;
}

#endif
