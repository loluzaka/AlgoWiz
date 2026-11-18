#ifndef LISTRENDERER_H
#define LISTRENDERER_H

// Linked list picture: boxed nodes joined by arrows, NULL, and HEAD / TAIL / cur / new / del
// labels. Nodes are placed by following the Next edges from the HEAD pointer; nodes that are
// not reachable yet (a freshly created node) are shown in a separate "not linked yet" row.

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../IRenderer.h"

using namespace std;

class ListRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const GlyphSet& g = ctx.glyphs;
        const Container* c = ctx.world.find(0);
        out.put(0, 0, ctx.containerName(0).empty() ? "Linked list" : ctx.containerName(0), th.header);
        if (!c || c->nodes.empty()) {
            out.put(0, 2, "HEAD " + g.right + " NULL   (the list is empty)", th.accent);
            return;
        }
        bool doubly = ctx.timeline.algorithm.find("doubly") != string::npos;
        auto nextOf = [&](int id) { return edgeTarget(c, id, Slot::Next); };
        auto prevOf = [&](int id) { return edgeTarget(c, id, Slot::Prev); };

        vector<int> order;
        set<int> seen;
        auto hp = c->pointers.find("HEAD");
        int cur = hp == c->pointers.end() ? -1 : hp->second;
        bool circular = false;
        while (cur >= 0 && c->nodes.count(cur)) {
            if (seen.count(cur)) { circular = true; break; }
            seen.insert(cur); order.push_back(cur);
            cur = nextOf(cur);
        }
        vector<int> loose;
        for (const auto& n : c->nodes) if (!seen.count(n.first)) loose.push_back(n.first);

        map<int, vector<string>> labels;
        for (const auto& p : c->pointers) if (!isMetaPointer(p.first) && p.second >= 0) labels[p.second].push_back(p.first);

        int maxLen = 1;
        for (const auto& n : c->nodes) maxLen = max(maxLen, textWidth(to_string(n.second.value)));
        int inner = maxLen + 2, total = inner + 2, arrowW = 5;
        int y = 2;
        int prefix = doubly ? 6 : 0;
        int perRow = max(1, (ctx.width - prefix) / (total + arrowW));

        auto drawRow = [&](const vector<int>& ids, bool attached) {
            for (size_t start = 0; start < ids.size(); start += perRow) {
                size_t end = min(ids.size(), start + perRow);
                bool firstRow = start == 0;
                int x0 = firstRow ? prefix : 0;
                if (firstRow && doubly && attached) { out.put(0, y + 1, "NULL", th.dim); out.put(5, y + 1, g.left, th.border); }
                bool anyBadge = false;
                for (size_t k = start; k < end; k++) {
                    int id = ids[k];
                    int x = x0 + (int)(k - start) * (total + arrowW);
                    Role role = ctx.roleOf(0, id);
                    Style st = role == Role::Default ? th.border : th.role(role).style;
                    drawBox(out, x, y, inner, to_string(c->nodes.at(id).value), g, st, role == Role::Default ? th.normal : st);
                    auto lb = labels.find(id);
                    string lab = lb != labels.end() ? g.arrowUp + joinLabels(lb->second) : "";
                    int nx = nextOf(id);
                    int ax = x + total;
                    if (attached && k + 1 < ids.size()) {
                        int following = ids[k + 1];
                        bool fwd = nx == following;
                        bool back = doubly && prevOf(following) == id;
                        string arrow = fwd && back ? g.arrowBoth : fwd ? g.arrowR : back ? g.arrowL : "";
                        if (k + 1 < end) out.put(ax + 1, y + 1, arrow, th.border);
                        else out.put(ax + 1, y + 1, g.ellipsis, th.dim);
                        if (nx >= 0 && !fwd) lab += (lab.empty() ? "" : " ") + string("next=") + to_string(c->nodes.count(nx) ? c->nodes.at(nx).value : nx);
                    } else if (attached) {                                  // last node of the list
                        if (nx < 0) out.put(ax + 1, y + 1, "NULL", th.dim);
                        else if (circular) out.put(ax + 1, y + 1, g.right + " HEAD", th.accent);
                    } else if (nx >= 0) {
                        lab += (lab.empty() ? "" : " ") + string("next=") + to_string(c->nodes.count(nx) ? c->nodes.at(nx).value : nx);
                    }
                    if (!lab.empty()) out.putCentered(x, total, y + 3, lab, th.accent);
                    string b = ctx.badgeOf(0, id);
                    if (!b.empty()) { out.putCentered(x, total, y + 4, b, st); anyBadge = true; }
                }
                y += 4 + (anyBadge ? 1 : 0) + 1;
            }
        };
        drawRow(order, true);
        if (!loose.empty()) {
            out.put(0, y, "Not linked into the list yet:", th.dim);
            y += 1;
            drawRow(loose, false);
        }
        if (circular) out.put(0, y, "(circular: the last node points back to HEAD)", th.dim);
    }

private:
    static int edgeTarget(const Container* c, int id, Slot s) {
        auto it = c->edges.find(id);
        if (it == c->edges.end()) return -1;
        for (const auto& e : it->second) if (e.slot == s) return e.to;
        return -1;
    }
};

#endif
