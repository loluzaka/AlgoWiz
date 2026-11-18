#ifndef RENDERERFACTORY_H
#define RENDERERFACTORY_H

#include <memory>

#include "IRenderer.h"
#include "array/ArrayRenderer.h"
#include "graph/GraphRenderer.h"
#include "hash/HashRenderer.h"
#include "heap/HeapRenderer.h"
#include "linear/QueueRenderer.h"
#include "linear/StackRenderer.h"
#include "list/ListRenderer.h"
#include "tree/TreeRenderer.h"

// One renderer per StructureKind. Plugins choose a kind in their metadata, so custom
// algorithms and structures reuse exactly the same renderers as the built-in ones.
inline unique_ptr<IRenderer> makeRenderer(StructureKind k) {
    switch (k) {
    case StructureKind::Array: return unique_ptr<IRenderer>(new ArrayRenderer());
    case StructureKind::List:  return unique_ptr<IRenderer>(new ListRenderer());
    case StructureKind::Stack: return unique_ptr<IRenderer>(new StackRenderer());
    case StructureKind::Queue: return unique_ptr<IRenderer>(new QueueRenderer());
    case StructureKind::Hash:  return unique_ptr<IRenderer>(new HashRenderer());
    case StructureKind::Tree:  return unique_ptr<IRenderer>(new TreeRenderer());
    case StructureKind::Heap:  return unique_ptr<IRenderer>(new HeapRenderer());
    case StructureKind::Graph: return unique_ptr<IRenderer>(new GraphRenderer());
    }
    return unique_ptr<IRenderer>(new ArrayRenderer());
}

#endif
