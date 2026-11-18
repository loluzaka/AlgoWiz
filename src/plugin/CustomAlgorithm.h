#ifndef CUSTOMALGORITHM_H
#define CUSTOMALGORITHM_H

// Interface a student implements to add an algorithm (spec section 47). Compare with the
// built-in Algorithm interface: this one is deliberately smaller - the plugin only describes
// itself and writes steps through a PluginContext. The adapter turns it into a real Algorithm.

#include "PluginContext.h"
#include "PluginMetadata.h"
#include "../engine/Input.h"

using namespace std;

class CustomAlgorithm {
public:
    virtual ~CustomAlgorithm() = default;
    virtual PluginMetadata metadata() const = 0;
    virtual void execute(const Input& input, PluginContext& ctx) = 0;
};

#endif
