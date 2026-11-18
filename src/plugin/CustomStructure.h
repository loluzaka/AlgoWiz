#ifndef CUSTOMSTRUCTURE_H
#define CUSTOMSTRUCTURE_H

// Interface for a custom data structure. It is driven by the same Command history as the
// built-in structures; `metadata().renders` chooses which renderer family draws it
// (array / list / tree / graph / table = hash ...).

#include "PluginContext.h"
#include "PluginMetadata.h"
#include "../engine/Input.h"

using namespace std;

class CustomStructure {
public:
    virtual ~CustomStructure() = default;
    virtual PluginMetadata metadata() const = 0;
    virtual void reset(const Input&, PluginContext&) {}                 // a new, empty structure
    virtual void addInitial(int value, PluginContext& ctx) { Command c; c.op = "add"; c.a = value; apply(c, ctx); }
    virtual void apply(const Command& cmd, PluginContext& ctx) = 0;     // one user operation
};

#endif
