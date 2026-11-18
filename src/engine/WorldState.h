#ifndef WORLDSTATE_H
#define WORLDSTATE_H

#include <map>
#include <string>
#include <vector>

#include "Step.h"

using namespace std;

struct NodeState {
    int    value = 0;
    string text;           // used when hasText (string sorting, hash tombstones, graph names)
    bool   hasText = false;
    string label;          // free annotation such as "bf=-1"
    bool operator==(const NodeState& o) const {
        return value == o.value && text == o.text && hasText == o.hasText && label == o.label;
    }
};

struct EdgeState {
    int  to = -1;
    Slot slot = Slot::None;
    int  weight = 1;
    bool operator==(const EdgeState& o) const { return to == o.to && slot == o.slot && weight == o.weight; }
};

// One lane of the picture: a main array, an auxiliary count array, a queue, ...
struct Container {
    int capacity = 0;                    // number of visible slots (arrays, stacks, tables)
    map<int, NodeState> nodes;
    map<int, vector<EdgeState>> edges;   // outgoing edges per node
    map<string, int> pointers;           // label -> node id (i, j, TOP, HEAD ...)
    map<int, Role> marks;                // persistent roles set by RoleChange deltas
    bool operator==(const Container& o) const {
        return capacity == o.capacity && nodes == o.nodes && edges == o.edges &&
               pointers == o.pointers && marks == o.marks;
    }
};

class WorldState {
public:
    map<int, Container> containers;

    void apply(const Delta& d);
    void apply(const vector<Delta>& ds);

    const Container* find(int container) const;
    Container&       get(int container);          // creates if missing

    Role markOf(int container, int id) const;
    bool operator==(const WorldState& o) const { return containers == o.containers; }
    string signature() const;                    // stable text dump, used by the tests
};

#endif
