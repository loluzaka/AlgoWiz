#ifndef ARRAYRENDERER_H
#define ARRAYRENDERER_H

// Indexed cells with i / j / mid style pointers, the shared "row of cells" drawing used by
// the array, queue, heap and graph (queue/stack) renderers, and the main array renderer.

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "../IRenderer.h"

using namespace std;

inline bool isMetaPointer(const string& name) { return name == "directed" || name == "from" || name == "to"; }

inline string cellText(const NodeState& n) { return n.hasText ? n.text : to_string(n.value); }

// Draws one container as rows of boxed cells. Returns the next free y.
//   caption  : heading drawn above the cells (container name)
//   withIndex: print the slot number above every cell
inline int drawArrayContainer(Canvas& out, const RenderContext& ctx, int container, int y, const string& caption, int x0 = 0, bool withIndex = true) {
    const Theme& th = ctx.theme;
    const GlyphSet& g = ctx.glyphs;
    const Container* c = ctx.world.find(container);
    if (!caption.empty()) { out.put(x0, y, caption, th.header); y++; }
    if (!c || (c->capacity == 0 && c->nodes.empty())) { out.put(x0, y, "(empty)", th.dim); return y + 2; }

    int slots = c->capacity;
    if (!c->nodes.empty()) slots = max(slots, c->nodes.rbegin()->first + 1);
    int maxLen = 1;
    for (const auto& n : c->nodes) maxLen = max(maxLen, textWidth(cellText(n.second)));
    int inner = max(3, maxLen) + 2;
    int total = inner + 2;
    int perRow = max(1, (ctx.width - x0) / (total + 1));

    map<int, vector<string>> labels;
    for (const auto& p : c->pointers) if (!isMetaPointer(p.first) && p.second >= 0) labels[p.second].push_back(p.first);
    bool anyBadge = false;
    if (th.badges)
        for (int i = 0; i < slots; i++) if (!ctx.badgeOf(container, i).empty()) anyBadge = true;

    for (int start = 0; start < slots; start += perRow) {
        int end = min(slots, start + perRow);
        for (int i = start; i < end; i++) {
            int x = x0 + (i - start) * (total + 1);
            auto it = c->nodes.find(i);
            bool present = it != c->nodes.end();
            Role role = ctx.roleOf(container, i);
            Style st = role == Role::Default ? (present ? th.border : th.dim) : th.role(role).style;
            if (withIndex) out.putCentered(x, total, y, to_string(i), th.dim);
            drawBox(out, x, y + 1, inner, present ? cellText(it->second) : "", g, st, role == Role::Default ? th.normal : st);
            auto lb = labels.find(i);
            if (lb != labels.end()) out.putCentered(x, total, y + 4, g.arrowUp + joinLabels(lb->second), th.accent);
            if (anyBadge) out.putCentered(x, total, y + 5, ctx.badgeOf(container, i), st);
        }
        y += 4 + (anyBadge ? 2 : 1);
    }
    return y + 1;
}

class ArrayRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        int y = 0;
        bool first = true;
        for (const auto& kv : ctx.world.containers) {
            const Container& c = kv.second;
            if (!first && c.capacity == 0 && c.nodes.empty()) continue;      // hide unused auxiliary rows
            string name = ctx.containerName(kv.first);
            if (name.empty()) name = kv.first == 0 ? "Array" : "Container " + to_string(kv.first);
            y = drawArrayContainer(out, ctx, kv.first, y, name);
            first = false;
        }
        if (first) out.put(0, 0, "(empty array)", ctx.theme.dim);
    }
};

#endif
