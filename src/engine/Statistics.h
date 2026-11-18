#ifndef STATISTICS_H
#define STATISTICS_H

#include <string>
#include <vector>
#include <utility>

#include "Step.h"

using namespace std;

// Live counters. Every counter is derived from OpType alone, so an algorithm
// never reports statistics by hand and the numbers can be recomputed at any step.
struct Statistics {
    long steps = 0;
    long comparisons = 0, swaps = 0, assignments = 0, accesses = 0, iterations = 0;
    long inserts = 0, deletes = 0;                 // for graphs: queue / stack operations
    long hashCalcs = 0, collisions = 0, probes = 0;
    long rotations = 0;
    long nodesVisited = 0, edgesExplored = 0, pointerMoves = 0;
    long heapifies = 0;

    void add(const Step& s) {
        steps++;
        switch (s.op) {
        case OpType::Compare:     comparisons++; accesses += 2; break;      // two reads
        case OpType::Swap:        swaps++;       accesses += 4; break;      // two reads + two writes
        case OpType::Assign:      assignments++; accesses += 2; break;      // one read + one write
        case OpType::Access:      accesses++; break;
        case OpType::Iterate:     iterations++; break;
        case OpType::Insert:      inserts++; break;
        case OpType::Delete:      deletes++; break;
        case OpType::Hash:        hashCalcs++; break;
        case OpType::Collision:   collisions++; break;
        case OpType::Probe:       probes++; break;
        case OpType::Rotate:      rotations++; break;
        case OpType::VisitNode:   nodesVisited++; break;
        case OpType::VisitEdge:   edgesExplored++; break;
        case OpType::PointerMove: pointerMoves++; break;
        case OpType::Heapify:     heapifies++; break;
        default: break;
        }
    }

    // Name/value rows that are non-zero, in a fixed order (info panel and report use this).
    vector<pair<string, long>> rows() const {
        vector<pair<string, long>> r;
        auto put = [&](const char* n, long v) { if (v) r.push_back({n, v}); };
        put("Comparisons", comparisons); put("Swaps", swaps); put("Assignments", assignments);
        put("Accesses", accesses); put("Iterations", iterations);
        put("Inserts", inserts); put("Deletes", deletes);
        put("Hash calcs", hashCalcs); put("Collisions", collisions); put("Probes", probes);
        put("Rotations", rotations); put("Nodes visited", nodesVisited);
        put("Edges explored", edgesExplored); put("Pointer moves", pointerMoves);
        put("Heapifies", heapifies);
        return r;
    }
};

#endif
