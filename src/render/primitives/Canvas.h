#ifndef CANVAS_H
#define CANVAS_H

// A character grid with a style per cell. Renderers and UI panels draw into Canvas objects;
// only at the very end is the grid turned into text (with or without ANSI escape codes).
// No other file in the project ever writes an escape sequence.

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

// Colours are palette indexes: -1 = terminal default, 0-7 normal, 8-15 bright, 16-255 xterm palette.
struct Style {
    int  fg = -1;
    int  bg = -1;
    bool bold = false, dim = false, inverse = false, underline = false;
    bool operator==(const Style& o) const {
        return fg == o.fg && bg == o.bg && bold == o.bold && dim == o.dim && inverse == o.inverse && underline == o.underline;
    }
    bool operator!=(const Style& o) const { return !(*this == o); }
    bool plain() const { return *this == Style(); }
};

struct Glyph {
    string ch = " ";      // one UTF-8 encoded character
    Style st;
};

// Splits UTF-8 text into its characters (one string per code point).
inline vector<string> utf8Chars(const string& s) {
    vector<string> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}
inline int textWidth(const string& s) { return (int)utf8Chars(s).size(); }

class Canvas {
public:
    Canvas(int width = 0, int height = 0) : w_(max(0, width)), h_(0) { ensureHeight(height); }

    int width() const { return w_; }
    int height() const { return h_; }
    void ensureHeight(int h) { if (h > h_) { cells_.resize((size_t)w_ * h); h_ = h; } }

    Glyph& at(int x, int y) { return cells_[(size_t)y * w_ + x]; }
    const Glyph& at(int x, int y) const { return cells_[(size_t)y * w_ + x]; }
    bool inside(int x, int y) const { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    // Writes text starting at (x, y). The canvas grows downwards automatically; columns outside are clipped.
    int put(int x, int y, const string& text, const Style& st = Style()) {
        if (y < 0) return x;
        ensureHeight(y + 1);
        for (const string& ch : utf8Chars(text)) {
            if (x >= 0 && x < w_) { Glyph& g = at(x, y); g.ch = ch; g.st = st; }
            x++;
        }
        return x;
    }
    void putChar(int x, int y, const string& ch, const Style& st = Style()) { put(x, y, ch, st); }
    // Centres text inside [x, x+width).
    void putCentered(int x, int width, int y, const string& text, const Style& st = Style()) {
        int tw = textWidth(text);
        put(x + max(0, (width - tw) / 2), y, text, st);
    }
    void fill(int x, int y, int w, int h, const string& ch = " ", const Style& st = Style()) {
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) put(x + i, y + j, ch, st);
    }
    // Straight line between two points using / \ | - characters (used for tree and graph edges).
    void line(int x0, int y0, int x1, int y1, const Style& st, bool unicode = false) {
        int dx = x1 - x0, dy = y1 - y0;
        int steps = (2 * abs(dy) >= abs(dx)) ? abs(dy) : abs(dx);       // one character per row on steep lines
        if (steps == 0) return;
        for (int i = 0; i <= steps; i++) {
            int x = x0 + (int)((double)dx * i / steps + (dx * i >= 0 ? 0.5 : -0.5));
            int y = y0 + (int)((double)dy * i / steps + (dy * i >= 0 ? 0.5 : -0.5));
            string ch;
            if (dx == 0) ch = unicode ? "\xE2\x94\x82" : "|";                      // |
            else if (dy == 0) ch = unicode ? "\xE2\x94\x80" : "-";                 // -
            else if (abs(dx) >= 4 * abs(dy)) ch = "-";          // a cell is about twice as tall as wide,
            else if (abs(dy) >= abs(dx)) ch = "|";              // so compare dy with dx / 2
            else if (2 * abs(dy) >= abs(dx)) ch = ((dx > 0) == (dy > 0)) ? "\\" : "/";
            else ch = ((dx > 0) == (dy > 0)) ? "\\" : "/";
            if (x >= 0 && x < w_) put(x, y, ch, st);
        }
    }
    // Copies the rectangle (sx, sy, w, h) of `src` to (dx, dy) of this canvas, clipped to this canvas.
    void blit(const Canvas& src, int sx, int sy, int w, int h, int dx, int dy) {
        for (int j = 0; j < h; j++) {
            int yy = sy + j;
            if (yy < 0 || yy >= src.h_ || dy + j < 0) continue;
            ensureHeight(dy + j + 1);
            for (int i = 0; i < w; i++) {
                int xx = sx + i, tx = dx + i;
                if (xx < 0 || xx >= src.w_ || tx < 0 || tx >= w_) continue;
                at(tx, dy + j) = src.at(xx, yy);
            }
        }
    }
    void blitAll(const Canvas& src, int dx, int dy) { blit(src, 0, 0, src.w_, src.h_, dx, dy); }

    // Number of rows that contain anything other than blank, unstyled cells.
    int usedHeight() const {
        for (int y = h_ - 1; y >= 0; y--)
            for (int x = 0; x < w_; x++) if (at(x, y).ch != " " || !at(x, y).st.plain()) return y + 1;
        return 0;
    }

    // Plain text (no escape codes), trailing spaces trimmed. Used by tests, --dump and monochrome screenshots.
    string toPlain(int rows = -1, bool trim = true) const {
        string out;
        int n = rows < 0 ? h_ : min(rows, h_);
        for (int y = 0; y < n; y++) {
            string line;
            for (int x = 0; x < w_; x++) line += at(x, y).ch;
            if (trim) while (!line.empty() && line.back() == ' ') line.pop_back();
            out += line + (y + 1 < n || trim ? "\n" : "");
        }
        return out;
    }

    // Text with ANSI SGR escape codes. Only changes of style emit codes; every row ends with a reset.
    string toAnsi(int rows = -1) const {
        string out;
        int n = rows < 0 ? h_ : min(rows, h_);
        for (int y = 0; y < n; y++) {
            Style cur;
            for (int x = 0; x < w_; x++) {
                const Glyph& g = at(x, y);
                if (g.st != cur) { out += sgr(g.st); cur = g.st; }
                out += g.ch;
            }
            if (!cur.plain()) out += "\x1b[0m";
            if (y + 1 < n) out += "\n";
        }
        return out;
    }

    static string sgr(const Style& s) {
        string c = "\x1b[0";
        if (s.bold) c += ";1";
        if (s.dim) c += ";2";
        if (s.underline) c += ";4";
        if (s.inverse) c += ";7";
        auto color = [&](int v, bool fg) {
            if (v < 0) return;
            if (v < 8) c += ";" + to_string((fg ? 30 : 40) + v);
            else if (v < 16) c += ";" + to_string((fg ? 90 : 100) + v - 8);
            else c += string(fg ? ";38;5;" : ";48;5;") + to_string(v);
        };
        color(s.fg, true);
        color(s.bg, false);
        return c + "m";
    }

private:
    int w_, h_;
    vector<Glyph> cells_;
};

#endif
