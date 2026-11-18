#ifndef DESCRIBE_H
#define DESCRIBE_H

// Human-readable names for operations, roles and whole steps. Used by the explanation
// bar, Quiz Mode (answer options) and the exported report. No Qt, no terminal.

#include <set>
#include <string>
#include <vector>

#include "Step.h"

using namespace std;

inline string opName(OpType op) {
    switch (op) {
    case OpType::Compare: return "COMPARE";        case OpType::Swap: return "SWAP";
    case OpType::Assign: return "ASSIGN";          case OpType::Insert: return "INSERT";
    case OpType::Delete: return "DELETE";          case OpType::NodeCreate: return "NODE_CREATE";
    case OpType::NodeDelete: return "NODE_DELETE"; case OpType::EdgeAdd: return "EDGE_ADD";
    case OpType::EdgeRemove: return "EDGE_REMOVE"; case OpType::Mark: return "MARK";
    case OpType::Unmark: return "UNMARK";          case OpType::Highlight: return "HIGHLIGHT";
    case OpType::Note: return "NOTE";              case OpType::Access: return "ACCESS";
    case OpType::PointerMove: return "POINTER_MOVE"; case OpType::VisitNode: return "VISIT_NODE";
    case OpType::VisitEdge: return "VISIT_EDGE";   case OpType::Rotate: return "ROTATE";
    case OpType::Hash: return "HASH";              case OpType::Collision: return "COLLISION";
    case OpType::Probe: return "PROBE";            case OpType::Heapify: return "HEAPIFY";
    case OpType::Partition: return "PARTITION";    case OpType::Merge: return "MERGE";
    case OpType::Iterate: return "ITERATE";
    }
    return "?";
}

inline string roleName(Role r) {
    switch (r) {
    case Role::Default: return "default";    case Role::Compared: return "compared";   case Role::Sorted: return "sorted";
    case Role::Pivot: return "pivot";        case Role::Found: return "found";         case Role::Active: return "active";
    case Role::Current: return "current";    case Role::Candidate: return "candidate"; case Role::Visited: return "visited";
    case Role::Queued: return "queued";      case Role::Frontier: return "frontier";   case Role::Path: return "path";
    case Role::Error: return "error";        case Role::Moved: return "moved";         case Role::Collision: return "collision";
    case Role::Eliminated: return "eliminated"; case Role::Range: return "range";      case Role::NotFound: return "not found";
    case Role::GroupA: return "group A";     case Role::GroupB: return "group B";
    }
    return "?";
}

// The entities a step is about: ids from its deltas first, then from its role tags.
inline vector<int> stepTargets(const Step& s) {
    vector<int> ids;
    set<int> seen;
    auto add = [&](int id) { if (id >= 0 && !seen.count(id)) { seen.insert(id); ids.push_back(id); } };
    for (const Delta& d : s.deltas) if (d.kind != Delta::Kind::Resize && d.kind != Delta::Kind::Clear && d.kind != Delta::Kind::PointerSet) add(d.target_id);
    for (const RoleTag& r : s.roles) add(r.target_id);
    for (const Delta& d : s.deltas) if (d.kind == Delta::Kind::PointerSet) add(d.target_id);
    return ids;
}

// e.g. "SWAP on positions 2, 3"
inline string describeStep(const Step& s) {
    string out = opName(s.op);
    vector<int> t = stepTargets(s);
    if (!t.empty()) {
        out += t.size() == 1 ? " on position " : " on positions ";
        for (size_t i = 0; i < t.size() && i < 4; i++) out += (i ? ", " : "") + to_string(t[i]);
        if (t.size() > 4) out += ", ...";
    }
    return out;
}

#endif
