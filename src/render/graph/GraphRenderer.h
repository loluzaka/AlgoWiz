#ifndef GRAPHRENDERER_H
#define GRAPHRENDERER_H

// Graph picture with three views (press V to cycle):
//   0 Adjacency matrix weights in a grid
//   1 Drawing          vertices on a circle, edges as lines with weights and arrowheads
//   2 Adjacency list   "A: B(4) C(2)"
// The BFS queue / DFS stack (container 1) is drawn below as a row of cells.
// DECISION: a circle layout is used instead of force-directed layout, because it is
// deterministic (the picture never jumps between steps) and never overlaps vertices.
// Dragging vertices is replaced by the three views, since a terminal has no mouse.

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "../IRenderer.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class GraphRenderer : public IRenderer {
public:
    int viewModeCount() const override { return 3; }
    string viewModeName(int m) const override { return m == 0 ? "adjacency matrix" : m == 1 ? "drawing" : "adjacency list"; }

    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const Container* c = ctx.world.find(0);
        bool directed = c && c->pointers.count("directed") && c->pointers.at("directed") == 1;
        out.put(0, 0, string(directed ? "Directed" : "Undirected") + " graph - view: " + viewModeName(ctx.viewMode % 3) + "   (V = change view)", th.header);
        if (!c || c->nodes.empty()) { out.put(0, 2, "(the graph has no vertices)", th.dim); return; }
        int from = -1, to = -1;
        auto f = c->pointers.find("from"); if (f != c->pointers.end()) from = f->second;
        auto t = c->pointers.find("to");   if (t != c->pointers.end()) to = t->second;

        int y = 2;
        switch (ctx.viewMode % 3) {
        case 0: y = drawMatrix(out, ctx, c, y); break;
        case 1: y = drawPicture(out, ctx, c, directed, from, to, y); break;
        default: y = drawList(out, ctx, c, y); break;
        }
        const Container* aux = ctx.world.find(1);
        if (aux && (aux->capacity > 0 || !aux->nodes.empty()))
            drawArrayContainer(out, ctx, 1, y + 1, ctx.containerName(1).empty() ? "Queue / stack" : ctx.containerName(1));
    }

private:
    static string nameOf(const Container* c, int id) { return cellText(c->nodes.at(id)); }

    int drawPicture(Canvas& out, const RenderContext& ctx, const Container* c, bool directed, int from, int to, int y0) {
        const Theme& th = ctx.theme;
        vector<int> ids;
        for (const auto& n : c->nodes) ids.push_back(n.first);
        int n = (int)ids.size();
        int rx = max(6, min(ctx.width / 2 - 8, 28)), ry = n <= 2 ? 3 : 6;
        int cx = rx + 6, cy = y0 + ry + 1;
        map<int, pair<int, int>> pos;
        for (int i = 0; i < n; i++) {
            double ang = 2.0 * 3.14159265358979 * i / n - 3.14159265358979 / 2;
            pos[ids[i]] = {cx + (int)lround(rx * cos(ang)), cy + (int)lround(ry * sin(ang))};
        }
        Canvas pic(cx + rx + 12, 2 * ry + 4);
        for (const auto& kv : c->edges) {
            int u = kv.first;
            if (!pos.count(u)) continue;
            for (const auto& e : kv.second) {
                int v = e.to;
                if (!pos.count(v)) continue;
                if (!directed && u > v) continue;                       // undirected: draw each edge once
                bool hot = (u == from && v == to) || (!directed && u == to && v == from);
                Style st = hot ? th.accent : th.dim;
                int x0 = pos[u].first, y0b = pos[u].second - y0, x1 = pos[v].first, y1 = pos[v].second - y0;
                shorten(x0, y0b, x1, y1);
                pic.line(x0, y0b, x1, y1, st);
                if (directed) {
                    int dx = x1 - x0, dy = y1 - y0;
                    string head = abs(dx) >= abs(dy) * 2 ? (dx > 0 ? ">" : "<") : abs(dy) >= abs(dx) * 2 ? (dy > 0 ? "v" : "^") : (dx > 0 ? (dy > 0 ? "\\" : "/") : (dy > 0 ? "/" : "\\"));
                    pic.put(x1, y1, head == "\\" || head == "/" ? (dy > 0 ? "v" : "^") : head, th.normal);
                }
                string w = to_string(e.weight);
                pic.put((x0 + x1) / 2 + 1, (y0b + y1) / 2, w, hot ? th.accent : th.normal);
            }
        }
        for (int id : ids) {
            Role role = ctx.roleOf(0, id);
            Style st = role == Role::Default ? th.normal : th.role(role).style;
            string txt = "(" + nameOf(c, id) + ")";
            pic.put(pos[id].first - 1, pos[id].second - y0, txt, st);
            string b = ctx.badgeOf(0, id);
            if (!b.empty()) pic.put(pos[id].first - textWidth(b) / 2, pos[id].second - y0 + 1, b, st);
        }
        out.blitAll(pic, 0, y0);
        return y0 + pic.height();
    }

    // Moves the end points of a line off the vertex symbols so "(A)" stays readable.
    static void shorten(int& x0, int& y0, int& x1, int& y1) {
        int dx = x1 - x0, dy = y1 - y0;
        if (abs(dx) + abs(dy) < 6) return;
        auto step = [](int d) { return d > 0 ? 1 : d < 0 ? -1 : 0; };
        if (abs(dy) * 2 >= abs(dx)) { y0 += step(dy); y1 -= step(dy); }
        else { x0 += 2 * step(dx); x1 -= 2 * step(dx); }
    }

    int drawList(Canvas& out, const RenderContext& ctx, const Container* c, int y) {
        const Theme& th = ctx.theme;
        out.put(0, y++, "Adjacency list (vertex: neighbour(weight) ...)", th.dim);
        for (const auto& n : c->nodes) {
            Role role = ctx.roleOf(0, n.first);
            Style st = role == Role::Default ? th.normal : th.role(role).style;
            string b = ctx.badgeOf(0, n.first);
            int x = out.put(0, y, nameOf(c, n.first) + ":", st);
            x = out.put(x + 1, y, "", st);
            auto e = c->edges.find(n.first);
            if (e == c->edges.end() || e->second.empty()) out.put(x, y, "-", th.dim);
            else for (const auto& ed : e->second) {
                bool hot = n.first == lookup(c, "from") && ed.to == lookup(c, "to");
                string item = c->nodes.count(ed.to) ? nameOf(c, ed.to) + "(" + to_string(ed.weight) + ")" : "?";
                x = out.put(x, y, item, hot ? th.accent : th.normal) + 1;
            }
            if (!b.empty()) out.put(max(x + 1, 40), y, b, st);
            y++;
        }
        return y;
    }

    static int lookup(const Container* c, const char* label) { auto it = c->pointers.find(label); return it == c->pointers.end() ? -2 : it->second; }

    int drawMatrix(Canvas& out, const RenderContext& ctx, const Container* c, int y) {
        const Theme& th = ctx.theme;
        vector<int> ids;
        for (const auto& n : c->nodes) ids.push_back(n.first);

        int rowWidth = textWidth("from") + 2;
        int cellWidth = 5;
        for (int id : ids) {
            rowWidth = max(rowWidth, textWidth(nameOf(c, id)) + 2);
            cellWidth = max(cellWidth, textWidth(nameOf(c, id)) + 2);
            auto edges = c->edges.find(id);
            if (edges != c->edges.end())
                for (const EdgeState& edge : edges->second)
                    if (c->nodes.count(edge.to))
                        cellWidth = max(cellWidth, textWidth(to_string(edge.weight)) + 2);
        }

        int matrixWidth = 1 + rowWidth + 1 + (int)ids.size() * (cellWidth + 1);
        Canvas matrix(matrixWidth, 0);
        int matrixY = 0;
        auto border = [&]() {
            string line = "+" + ctx.glyphs.repeat(ctx.glyphs.h, rowWidth) + "+";
            for (size_t i = 0; i < ids.size(); i++)
                line += ctx.glyphs.repeat(ctx.glyphs.h, cellWidth) + "+";
            matrix.put(0, matrixY++, line, th.border);
        };
        auto row = [&](const string& label, const Style& labelStyle, const vector<string>& values,
                       const vector<Style>& valueStyles) {
            matrix.put(0, matrixY, ctx.glyphs.v, th.border);
            matrix.putCentered(1, rowWidth, matrixY, label, labelStyle);
            int x = rowWidth + 1;
            matrix.put(x++, matrixY, ctx.glyphs.v, th.border);
            for (size_t i = 0; i < values.size(); i++) {
                matrix.putCentered(x, cellWidth, matrixY, values[i], valueStyles[i]);
                x += cellWidth;
                matrix.put(x++, matrixY, ctx.glyphs.v, th.border);
            }
            matrixY++;
        };

        out.put(0, y++, "Adjacency matrix (rows: from, columns: to; . = no edge)", th.dim);
        vector<string> columnNames;
        vector<Style> columnStyles(ids.size(), th.accent);
        for (int id : ids) columnNames.push_back(nameOf(c, id));
        border();
        row("from", th.accent, columnNames, columnStyles);
        border();
        for (int u : ids) {
            Role role = ctx.roleOf(0, u);
            Style labelStyle = role == Role::Default ? th.accent : th.role(role).style;
            vector<string> values;
            vector<Style> styles;
            for (int v : ids) {
                bool hasEdge = false;
                int weight = 0;
                auto e = c->edges.find(u);
                if (e != c->edges.end())
                    for (const auto& ed : e->second)
                        if (ed.to == v) { hasEdge = true; weight = ed.weight; break; }
                bool hot = u == lookup(c, "from") && v == lookup(c, "to");
                values.push_back(hasEdge ? to_string(weight) : ctx.glyphs.dot);
                styles.push_back(hot ? th.accent : (hasEdge ? th.normal : th.dim));
            }
            row(nameOf(c, u), labelStyle, values, styles);
            border();
        }

        int pan = max(0, min(ctx.panX, matrixWidth - out.width()));
        out.blit(matrix, pan, 0, out.width(), matrix.height(), 0, y);
        y += matrix.height();
        if (matrixWidth > out.width()) out.put(0, y++, "scroll with [ and ]", th.dim);
        return y;
    }
};

#endif
