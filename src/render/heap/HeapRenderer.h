#ifndef HEAPRENDERER_H
#define HEAPRENDERER_H

// Heap: the same data shown twice - as a tree (parent of i is (i-1)/2) and as the array it is
// stored in. Side by side on wide terminals, stacked on narrow ones.

#include <string>

#include "../tree/TreeLayout.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class HeapRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const Container* c = ctx.world.find(0);
        out.put(0, 0, "Heap", th.header);
        if (!c || c->nodes.empty()) { out.put(0, 2, "(the heap is empty)", th.dim); return; }
        int n = c->nodes.rbegin()->first + 1;

        TreeLayout L;
        map<int, vector<string>> ptr;
        for (const auto& p : c->pointers) if (!isMetaPointer(p.first) && p.second >= 0) ptr[p.second].push_back(p.first);
        for (int i = 0; i < n; i++) {
            auto it = c->nodes.find(i);
            if (it == c->nodes.end()) continue;
            LayoutItem li;
            li.id = i; li.label = cellText(it->second); li.role = ctx.roleOf(0, i);
            li.left = 2 * i + 1 < n ? 2 * i + 1 : -1;
            li.right = 2 * i + 2 < n ? 2 * i + 2 : -1;
            li.sub = "[" + to_string(i) + "]";
            auto pl = ptr.find(i);
            if (pl != ptr.end()) li.tag = "^" + joinLabels(pl->second);
            L.items[i] = li;
        }
        L.roots.push_back(0);

        bool sideBySide = ctx.width >= 120;
        if (sideBySide) {
            int leftW = ctx.width * 3 / 5;
            out.put(0, 1, "Tree view", th.dim);
            drawTreeLayout(out, ctx, L, 0, 2, leftW);
            RenderContext arrCtx = ctx;
            arrCtx.width = ctx.width - leftW - 2;
            Canvas arr(arrCtx.width, 0);
            drawArrayContainer(arr, arrCtx, 0, 0, "Array view");
            out.blitAll(arr, leftW + 2, 1);
        } else {
            out.put(0, 1, "Tree view", th.dim);
            int rows = drawTreeLayout(out, ctx, L, 0, 2, ctx.width);
            drawArrayContainer(out, ctx, 0, 2 + rows + 1, "Array view");
        }
    }
};

#endif
