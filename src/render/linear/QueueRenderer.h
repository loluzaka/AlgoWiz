#ifndef QUEUERENDERER_H
#define QUEUERENDERER_H

// Horizontal queue (linear or circular): indexed slots with FRONT and REAR arrows.

#include <string>

#include "../IRenderer.h"
#include "../array/ArrayRenderer.h"

using namespace std;

class QueueRenderer : public IRenderer {
public:
    void draw(Canvas& out, const RenderContext& ctx) override {
        const Theme& th = ctx.theme;
        const Container* c = ctx.world.find(0);
        bool circular = ctx.timeline.algorithm.find("circular") != string::npos;
        string title = string(circular ? "Circular queue" : "Queue") + " (capacity " + to_string(c ? c->capacity : 0) + ")";
        int y = drawArrayContainer(out, ctx, 0, 0, title);
        int front = -1, rear = -1;
        if (c) {
            auto f = c->pointers.find("FRONT"); if (f != c->pointers.end()) front = f->second;
            auto r = c->pointers.find("REAR");  if (r != c->pointers.end()) rear = r->second;
        }
        out.put(0, y, front < 0 ? "FRONT = -    REAR = -    (empty)" : "FRONT = " + to_string(front) + "    REAR = " + to_string(rear), th.accent);
        out.put(0, y + 1, circular ? "FRONT removes, REAR adds; both indexes wrap around."
                                   : "FRONT removes, REAR adds; freed cells are not reused.", th.dim);
    }
};

#endif
