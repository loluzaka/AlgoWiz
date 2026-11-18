#ifndef SHAPES_H
#define SHAPES_H

// Shared drawing primitives: the glyph set (Unicode / ASCII fallback) and helpers that
// draw a cell, a box, an arrow and a label. Every renderer builds its picture from these,
// which keeps the visual language identical across arrays, lists, trees and graphs (spec 32).

#include <string>

#include "Canvas.h"

using namespace std;

struct GlyphSet {
    bool unicode = false;
    string h = "-", v = "|", tl = "+", tr = "+", bl = "+", br = "+";
    string arrowR = "-->", arrowL = "<--", arrowBoth = "<->", arrowUp = "^", arrowDown = "v", dot = ".", cross = "x";
    string right = ">", left = "<", ellipsis = "...";

    static GlyphSet ascii() { return GlyphSet(); }
    static GlyphSet utf8() {
        GlyphSet g;
        g.unicode = true;
        g.h = "\xE2\x94\x80"; g.v = "\xE2\x94\x82";
        g.tl = "\xE2\x94\x8C"; g.tr = "\xE2\x94\x90"; g.bl = "\xE2\x94\x94"; g.br = "\xE2\x94\x98";
        g.arrowR = "\xE2\x94\x80\xE2\x96\xB6"; g.arrowL = "\xE2\x97\x80\xE2\x94\x80"; g.arrowBoth = "\xE2\x97\x80\xE2\x96\xB6";
        g.arrowUp = "\xE2\x96\xB2"; g.arrowDown = "\xE2\x96\xBC"; g.dot = "\xC2\xB7"; g.cross = "\xC3\x97";
        g.right = "\xE2\x96\xB6"; g.left = "\xE2\x97\x80"; g.ellipsis = "\xE2\x80\xA6";
        return g;
    }
    string repeat(const string& s, int n) const { string r; for (int i = 0; i < n; i++) r += s; return r; }
};

// A 3-row box:  top border / text row / bottom border.  `inner` = number of characters between the borders.
inline void drawBox(Canvas& c, int x, int y, int inner, const string& text, const GlyphSet& g, const Style& border, const Style& textStyle) {
    c.put(x, y, g.tl + g.repeat(g.h, inner) + g.tr, border);
    c.put(x, y + 1, g.v, border);
    c.put(x + inner + 1, y + 1, g.v, border);
    c.putCentered(x + 1, inner, y + 1, text, textStyle);
    c.put(x, y + 2, g.bl + g.repeat(g.h, inner) + g.br, border);
}

// A box drawn with the role style on border AND text (so highlighted entities stand out).
inline void drawRoleBox(Canvas& c, int x, int y, int inner, const string& text, const GlyphSet& g, const Style& roleStyle) {
    drawBox(c, x, y, inner, text, g, roleStyle, roleStyle);
}

// Draws `label` centred under/over a point and returns nothing; used for HEAD, TOP, i, j, mid ...
inline void drawLabel(Canvas& c, int centerX, int y, const string& label, const Style& st) {
    int w = textWidth(label);
    c.put(centerX - w / 2, y, label, st);
}

// Horizontal arrow of `len` cells ending in a head, starting at (x, y).
inline void drawArrowH(Canvas& c, int x, int y, int len, bool pointRight, const GlyphSet& g, const Style& st) {
    if (len <= 0) return;
    string body = g.repeat(g.h, len - 1);
    if (pointRight) c.put(x, y, body + g.right, st);
    else            c.put(x, y, g.left + body, st);
}

// Joins labels that sit on the same cell, e.g. {"i","j"} -> "i,j".
inline string joinLabels(const vector<string>& v) {
    string s;
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + v[i];
    return s;
}

#endif
