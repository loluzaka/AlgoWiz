#ifndef TREELAYOUT_H
#define TREELAYOUT_H

// Automatic binary-tree layout used by the tree and heap renderers.
//
// Algorithm (in-order slot layout): walk the tree in-order and give every node the next free
// horizontal slot. Because in-order visits the left subtree completely before the right one,
// two subtrees can never overlap. The depth gives the row. Each node is drawn as
//        (label)        <- node, styled by its role
//         sub           <- small text such as the AVL balance factor "bf=+1"
//         ^tag          <- pointer names (cur, root ...)
//        [CMP]          <- role badge (only in themes that use non-colour cues)
// and the edges to the children are drawn as   ____/ \____   with the diagonals touching the
// children. Pictures wider than the pane are cropped with a horizontal scroll offset (panX).

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../IRenderer.h"

using namespace std;

struct LayoutItem {
    int id = -1;
    string label;           // text inside the node, e.g. "50"
    string sub;             // small text under the node, e.g. "bf=+1" or "[3]"
    string tag;             // pointer names, e.g. "^cur"
    Role role = Role::Default;
    int left = -1, right = -1;
};

struct TreeLayout {
    map<int, LayoutItem> items;
    vector<int> roots;
};

// Draws the forest and returns the number of rows used.
inline int drawTreeLayout(Canvas& out, const RenderContext& ctx, const TreeLayout& L, int x0, int y0, int availWidth) {
    const Theme& th = ctx.theme;
    if (L.items.empty()) return 0;

    // 1. slot per node (in-order), depth per node
    map<int, int> slot, depth;
    int counter = 0, maxDepth = 0;
    function<void(int, int)> walk = [&](int id, int d) {
        if (id < 0 || !L.items.count(id) || slot.count(id)) return;
        const LayoutItem& it = L.items.at(id);
        walk(it.left, d + 1);
        slot[id] = counter++;
        depth[id] = d;
        maxDepth = max(maxDepth, d);
        walk(it.right, d + 1);
    };
    for (int r : L.roots) { walk(r, 0); counter++; }               // blank slot between separate trees

    // 2. geometry
    bool anySub = false, anyTag = false, anyBadge = false;
    int pitch = 6;
    for (const auto& kv : L.items) {
        const LayoutItem& it = kv.second;
        if (!it.sub.empty()) anySub = true;
        if (!it.tag.empty()) anyTag = true;
        if (!th.badge(it.role).empty()) anyBadge = true;
        pitch = max(pitch, textWidth(it.label) + 4);
        pitch = max(pitch, min(textWidth(it.sub), 7) + 1);
    }
    int subRows = (anySub ? 1 : 0) + (anyTag ? 1 : 0) + (anyBadge ? 1 : 0);
    int stride = 1 + subRows + 2;
    int wideW = pitch * max(1, counter) + 2;
    Canvas wide(wideW, (maxDepth + 1) * stride);
    auto cx = [&](int id) { return slot.at(id) * pitch + pitch / 2 + 1; };
    auto cy = [&](int id) { return depth.at(id) * stride; };

    // 3. edges first (dim), then nodes on top
    for (const auto& kv : L.items) {
        if (!slot.count(kv.first)) continue;
        int px = cx(kv.first), y1 = cy(kv.first) + 1 + subRows, y2 = y1 + 1;
        for (int child : {kv.second.left, kv.second.right}) {
            if (child < 0 || !slot.count(child)) continue;
            int c = cx(child), dx = c - px;
            if (dx == 0) { wide.put(px, y1, "|", th.dim); wide.put(px, y2, "|", th.dim); continue; }
            bool leftSide = dx < 0;
            string diag = leftSide ? "/" : "\\";
            wide.put(px + (leftSide ? -1 : 1), y1, diag, th.dim);
            if (abs(dx) > 2) {
                int a = leftSide ? c + 1 : px + 2, b = leftSide ? px - 2 : c - 1;
                for (int x = a; x <= b; x++) wide.put(x, y1, "_", th.dim);
                wide.put(c, y2, diag, th.dim);
            }
        }
    }
    for (const auto& kv : L.items) {
        if (!slot.count(kv.first)) continue;
        const LayoutItem& it = kv.second;
        RoleStyle rs = th.role(it.role);
        Style st = it.role == Role::Default ? th.normal : rs.style;
        string txt = "(" + it.label + ")";
        int x = cx(it.id), y = cy(it.id);
        wide.put(x - textWidth(txt) / 2, y, txt, st);
        int row = y + 1;
        if (anySub) { if (!it.sub.empty()) wide.put(x - textWidth(it.sub) / 2, row, it.sub, th.accent); row++; }
        if (anyTag) { if (!it.tag.empty()) wide.put(x - textWidth(it.tag) / 2, row, it.tag, th.accent); row++; }
        if (anyBadge) { if (!rs.badge.empty()) wide.put(x - textWidth(rs.badge) / 2, row, rs.badge, st); }
    }

    // 4. crop with scroll offset
    int pan = max(0, min(ctx.panX, wideW - availWidth));
    int rows = wide.height();
    out.blit(wide, pan, 0, availWidth, rows, x0, y0);
    bool wideTree = wideW > availWidth;
    if (wideTree) out.put(x0, y0 + rows, (pan > 0 ? "< " : "  ") + string("scroll with [ and ]") + (pan + availWidth < wideW ? " >" : ""), th.dim);
    return rows + (wideTree ? 1 : 0);
}

#endif
