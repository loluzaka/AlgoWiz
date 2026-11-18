#ifndef IRENDERER_H
#define IRENDERER_H

#include <memory>
#include <string>
#include <vector>

#include "../engine/Step.h"
#include "../engine/WorldState.h"
#include "primitives/Canvas.h"
#include "primitives/Shapes.h"
#include "theme/Theme.h"

using namespace std;

// Everything a renderer may look at. A renderer is STATELESS: it draws the world at the
// current position of the engine, so rewinding needs no applyStep/undoStep pair (the
// "pointer rewind" model in Larping.txt). That is the one deliberate change to the Day-1
// IRenderer contract: draw() replaces applyStep()/undoStep()/reset().
struct RenderContext {
    const WorldState& world;
    const Step* step;                  // step just executed (nullptr at position 0)
    const Timeline& timeline;
    const Theme& theme;
    const GlyphSet& glyphs;
    int width = 80;                    // columns available
    int viewMode = 0;                  // renderer specific (graph: matrix / drawing / list)
    int panX = 0;                      // horizontal scroll for wide pictures

    RenderContext(const WorldState& w, const Step* s, const Timeline& t, const Theme& th, const GlyphSet& g)
        : world(w), step(s), timeline(t), theme(th), glyphs(g) {}

    // Transient role from the current step wins over the persistent mark.
    Role roleOf(int container, int id) const {
        if (step)
            for (auto it = step->roles.rbegin(); it != step->roles.rend(); ++it)
                if (it->container == container && it->target_id == id && it->role != Role::Default) return it->role;
        return world.markOf(container, id);
    }
    Style styleOf(int container, int id) const { return theme.role(roleOf(container, id)).style; }
    string badgeOf(int container, int id) const { return theme.badge(roleOf(container, id)); }
    string containerName(int c) const { return c >= 0 && c < (int)timeline.containerNames.size() ? timeline.containerNames[c] : ""; }
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    // Draws the picture into `out` (which grows downwards as needed).
    virtual void draw(Canvas& out, const RenderContext& ctx) = 0;
    virtual int viewModeCount() const { return 1; }
    virtual string viewModeName(int) const { return ""; }
};

#endif
