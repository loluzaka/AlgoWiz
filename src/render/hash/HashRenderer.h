#ifndef HASHRENDERER_H
#define HASHRENDERER_H

// Hash table: an index column, one row per slot, the probe marker, collision highlight,
// tombstones (DEL) and - for chaining - the linked chain after each slot.

#include <string>

#include "../IRenderer.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class HashRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const GlyphSet& g = ctx.glyphs;
        const Container* c = ctx.world.find(0);
        out.put(0, 0, "Hash table  (hash(key) = key mod " + to_string(c ? c->capacity : 0) + ")", th.header);
        if (!c || c->capacity == 0) { out.put(0, 2, "(no table)", th.dim); return; }
        int probe = -1;
        auto pi = c->pointers.find("probe");
        if (pi != c->pointers.end()) probe = pi->second;
        int maxLen = 3;
        for (const auto& n : c->nodes) maxLen = max(maxLen, textWidth(cellText(n.second)));
        int inner = maxLen + 2;
        out.put(0, 1, "idx", th.dim);
        int y = 2;
        for (int i = 0; i < c->capacity; i++, y++) {
            auto it = c->nodes.find(i);
            bool present = it != c->nodes.end();
            Role role = ctx.roleOf(0, i);
            Style st = role == Role::Default ? (present ? th.normal : th.dim) : th.role(role).style;
            Style border = role == Role::Default ? th.border : st;
            out.putCentered(0, 3, y, to_string(i), th.dim);
            out.put(4, y, g.v, border);
            out.putCentered(5, inner, y, present ? cellText(it->second) : g.dot, st);
            out.put(5 + inner, y, g.v, border);
            int x = 7 + inner;
            // chain (chaining only): follow Next edges from the slot
            int cur = i;
            for (int guard = 0; guard < 12; guard++) {
                auto ei = c->edges.find(cur);
                if (ei == c->edges.end()) break;
                int nxt = -1;
                for (const auto& e : ei->second) if (e.slot == Slot::Next) nxt = e.to;
                if (nxt < 0 || !c->nodes.count(nxt)) break;
                out.put(x, y, g.arrowR, th.border);
                x += textWidth(g.arrowR) + 1;
                Role cr = ctx.roleOf(0, nxt);
                Style cs = cr == Role::Default ? th.normal : th.role(cr).style;
                string txt = "[" + cellText(c->nodes.at(nxt)) + "]";
                out.put(x, y, txt, cs);
                x += textWidth(txt) + 1;
                string cb = ctx.badgeOf(0, nxt);
                if (!cb.empty()) { out.put(x, y, cb, cs); x += textWidth(cb) + 1; }
                cur = nxt;
            }
            if (i == probe) { out.put(x, y, g.left + " probe", th.accent); x += 8; }
            string b = ctx.badgeOf(0, i);
            if (!b.empty()) out.put(x, y, b, st);
        }
        bool tomb = false;
        for (const auto& n : c->nodes) if (n.second.hasText && n.second.text == "DEL") tomb = true;
        if (tomb) out.put(0, y + 1, "DEL = deleted slot (tombstone): searches keep probing past it, inserts may reuse it.", th.dim);
    }
};

#endif
