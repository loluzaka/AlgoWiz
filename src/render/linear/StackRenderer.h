#ifndef STACKRENDERER_H
#define STACKRENDERER_H

// Vertical stack: slot 0 at the bottom, TOP arrow beside the top element, empty slots dimmed.

#include <string>

#include "../IRenderer.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class StackRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const GlyphSet& g = ctx.glyphs;
        const Container* c = ctx.world.find(0);
        int cap = c ? c->capacity : 0;
        out.put(0, 0, "Stack (capacity " + to_string(cap) + ")", th.header);
        int top = -1;
        if (c) { auto it = c->pointers.find("TOP"); if (it != c->pointers.end()) top = it->second; }
        out.put(0, 1, "TOP = " + to_string(top) + (top < 0 ? "  (empty)" : ""), th.accent);
        int maxLen = 3;
        if (c) for (const auto& n : c->nodes) maxLen = max(maxLen, textWidth(cellText(n.second)));
        int inner = maxLen + 2, x0 = 10;
        out.put(x0, 2, g.tl + g.repeat(g.h, inner) + g.tr, th.border);
        int y = 3;
        for (int i = cap - 1; i >= 0; i--, y++) {
            bool present = c && c->nodes.count(i);
            Role role = ctx.roleOf(0, i);
            Style st = role == Role::Default ? (present ? th.normal : th.dim) : th.role(role).style;
            out.putCentered(0, 3, y, to_string(i), th.dim);
            if (i == top) out.put(3, y, "TOP" + g.right, th.accent);
            out.put(x0, y, g.v, th.border);
            out.put(x0 + inner + 1, y, g.v, th.border);
            out.putCentered(x0 + 1, inner, y, present ? cellText(c->nodes.at(i)) : "", st);
            string b = ctx.badgeOf(0, i);
            if (!b.empty()) out.put(x0 + inner + 3, y, b, st);
        }
        out.put(x0, y, g.bl + g.repeat(g.h, inner) + g.br, th.border);
        if (cap == 0) out.put(0, 3, "(no capacity)", th.dim);
    }
};

#endif
