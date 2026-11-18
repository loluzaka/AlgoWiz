#ifndef TREERENDERER_H
#define TREERENDERER_H

// Binary tree / BST / AVL picture built from the recorded nodes and Left / Right edges.
// Roots are the nodes nobody points to (during an AVL rotation the forest briefly has
// two roots, which the layout handles). AVL balance factors come from node labels.

#include <set>
#include <string>

#include "TreeLayout.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class TreeRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        out.put(0, 0, ctx.containerName(0).empty() ? "Tree" : ctx.containerName(0), th.header);
        const Container* c = ctx.world.find(0);
        if (!c || c->nodes.empty()) { out.put(0, 2, "(the tree is empty)   root = NULL", th.dim); return; }

        TreeLayout L;
        set<int> hasParent;
        map<int, vector<string>> ptr;
        for (const auto& p : c->pointers) if (!isMetaPointer(p.first) && p.second >= 0) ptr[p.second].push_back(p.first);
        for (const auto& n : c->nodes) {
            LayoutItem it;
            it.id = n.first;
            it.label = cellText(n.second);
            it.role = ctx.roleOf(0, n.first);
            auto e = c->edges.find(n.first);
            if (e != c->edges.end())
                for (const auto& x : e->second) {
                    if (!c->nodes.count(x.to)) continue;
                    if (x.slot == Slot::Left) it.left = x.to;
                    if (x.slot == Slot::Right) it.right = x.to;
                    if (x.slot == Slot::Left || x.slot == Slot::Right) hasParent.insert(x.to);
                }
            it.sub = n.second.label;
            auto pl = ptr.find(n.first);
            if (pl != ptr.end()) it.tag = "^" + joinLabels(pl->second);
            L.items[n.first] = it;
        }
        // the "root" pointer decides which tree is drawn first
        auto rp = c->pointers.find("root");
        if (rp != c->pointers.end() && L.items.count(rp->second)) L.roots.push_back(rp->second);
        for (const auto& n : c->nodes) if (!hasParent.count(n.first) && (L.roots.empty() || L.roots[0] != n.first)) L.roots.push_back(n.first);
        drawTreeLayout(out, ctx, L, 0, 2, ctx.width);
    }
};

#endif
