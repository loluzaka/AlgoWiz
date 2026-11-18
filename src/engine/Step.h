#ifndef STEP_H
#define STEP_H

#include <vector>
#include <string>
#include <cstdint>

#include "OpType.h"
#include "Role.h"

using namespace std;

// Which child / link an edge represents. None = plain (graph) edge.
enum class Slot { None, Left, Right, Next, Prev };

// Which renderer family draws a Timeline.
enum class StructureKind { Array, List, Stack, Queue, Hash, Tree, Heap, Graph };

struct Delta {
    enum class Kind { ValueChange, NodeCreate, NodeDelete, EdgeAdd, EdgeRemove, RoleChange,
                      // appended in the contract amendment
                      Resize, PointerSet, Label, Clear };
    Kind kind = Kind::ValueChange;
    int  target_id = -1;
    int  new_value = 0;
    int  edge_to = -1;
    int    container = 0;          // which container (lane) target_id lives in
    Slot   slot = Slot::None;      // left / right / next / prev for edges
    int    weight = 1;             // edge weight (graphs)
    string text;                   // string value, pointer label or node label

    static Delta value(int id, int v, int c = 0) {
        Delta d; d.kind = Kind::ValueChange; d.target_id = id; d.new_value = v; d.container = c; return d;
    }
    static Delta valueText(int id, const string& s, int c = 0) {
        Delta d; d.kind = Kind::ValueChange; d.target_id = id; d.text = s; d.container = c; return d;
    }
    static Delta create(int id, int v, int c = 0) {
        Delta d; d.kind = Kind::NodeCreate; d.target_id = id; d.new_value = v; d.container = c; return d;
    }
    static Delta createText(int id, const string& s, int c = 0) {
        Delta d; d.kind = Kind::NodeCreate; d.target_id = id; d.text = s; d.container = c; return d;
    }
    static Delta destroy(int id, int c = 0) {
        Delta d; d.kind = Kind::NodeDelete; d.target_id = id; d.container = c; return d;
    }
    static Delta edgeAdd(int from, int to, Slot s = Slot::None, int w = 1, int c = 0) {
        Delta d; d.kind = Kind::EdgeAdd; d.target_id = from; d.edge_to = to; d.slot = s; d.weight = w; d.container = c; return d;
    }
    static Delta edgeRemove(int from, int to, Slot s = Slot::None, int c = 0) {
        Delta d; d.kind = Kind::EdgeRemove; d.target_id = from; d.edge_to = to; d.slot = s; d.container = c; return d;
    }
    static Delta mark(int id, Role r, int c = 0) {
        Delta d; d.kind = Kind::RoleChange; d.target_id = id; d.new_value = (int)r; d.container = c; return d;
    }
    static Delta resize(int n, int c = 0) {
        Delta d; d.kind = Kind::Resize; d.new_value = n; d.container = c; return d;
    }
    static Delta pointer(const string& label, int id, int c = 0) {
        Delta d; d.kind = Kind::PointerSet; d.text = label; d.target_id = id; d.container = c; return d;
    }
    static Delta label(int id, const string& s, int c = 0) {
        Delta d; d.kind = Kind::Label; d.target_id = id; d.text = s; d.container = c; return d;
    }
    static Delta clear(int c = 0) {
        Delta d; d.kind = Kind::Clear; d.container = c; return d;
    }
};

// Transient visual role of ONE entity during ONE step.
struct RoleTag {
    int  target_id = -1;
    Role role = Role::Default;
    int  container = 0;
};

struct Step {
    uint64_t            seq = 0;
    OpType              op = OpType::Note;
    string              operation;       // structure command that produced this step, if any
    vector<Delta>       deltas;
    vector<RoleTag>     roles;      // was vector<Role>: now says WHICH entity gets which role
    string              note;
    float               progress = -1.0f;   // -1 = "engine fills in i/(n-1)"
    int                 codeLine = -1;      // index into AlgorithmInfo::pseudocode, -1 = none
};

struct Timeline {
    string              algorithm;
    vector<Step>        steps;
    // ---- appended in the contract amendment ----
    StructureKind       structure = StructureKind::Array;
    vector<Delta>       initial;            // deltas applied to an empty world to build state 0
    vector<string>      containerNames;     // caption of container 0,1,2...
    string              inputDescription;
    double              runtimeMs = 0.0;    // wall time spent generating the steps
    string              error;              // non-empty = run rejected / failed
};

#endif
