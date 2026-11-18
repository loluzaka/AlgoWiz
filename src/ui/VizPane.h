#ifndef VIZPANE_H
#define VIZPANE_H

// The "canvas" of the Qt plan (QGraphicsView with zoom / pan): draws one engine's current
// state with the right renderer into a rectangle of a screen Canvas and scrolls it
// vertically (panY) and horizontally (panX, used by wide trees). Shared by the visualizer
// and Race Mode.

#include <memory>

#include "../engine/ExecutionEngine.h"
#include "../render/RendererFactory.h"

using namespace std;

class VizPane {
public:
    int panX = 0, panY = 0, viewMode = 0;

    void setStructure(StructureKind k) {
        if (!renderer_ || kind_ != k) { renderer_ = makeRenderer(k); kind_ = k; panX = panY = 0; viewMode = 0; }
    }
    int viewModeCount() const { return renderer_ ? renderer_->viewModeCount() : 1; }
    string viewModeName() const { return renderer_ ? renderer_->viewModeName(viewMode) : ""; }
    void cycleView() { viewMode = (viewMode + 1) % max(1, viewModeCount()); }

    void draw(Canvas& dst, int x, int y, int w, int h, const ExecutionEngine& e, const Theme& th, const GlyphSet& g) {
        if (w < 10 || h < 3) return;
        setStructure(e.timeline().structure);
        RenderContext ctx(e.state(), e.currentStep(), e.timeline(), th, g);
        ctx.width = w; ctx.viewMode = viewMode; ctx.panX = panX;
        Canvas tmp(w, 0);
        renderer_->draw(tmp, ctx);
        int used = tmp.usedHeight();
        panY = max(0, min(panY, max(0, used - h)));
        dst.blit(tmp, 0, panY, w, h, x, y);
        if (panY > 0) dst.put(x + w - 12, y, "^ more (Up)", th.dim);
        if (used > panY + h) dst.put(x + w - 14, y + h - 1, "v more (Down)", th.dim);
    }

    // Keys shared by every screen that shows a pane. Returns true if the key was used.
    bool handleKey(int key) {
        switch (key) {
        case K_UP: if (panY > 0) panY--; return true;
        case K_DOWN: panY++; return true;
        case '[': panX = max(0, panX - 8); return true;
        case ']': panX += 8; return true;
        case 'v': case 'V': cycleView(); return true;
        default: return false;
        }
    }

private:
    unique_ptr<IRenderer> renderer_;
    StructureKind kind_ = StructureKind::Array;
};

#endif
