#ifndef STEPVALIDATOR_H
#define STEPVALIDATOR_H

// Rejects malformed output from plugins BEFORE it reaches the engine or a renderer.
// Built-in algorithms are trusted; plugin timelines are not.

#include <cmath>
#include <string>

#include "../engine/Step.h"

using namespace std;

struct StepLimits {
    size_t maxSteps = 200000;
    int maxContainer = 15;
    int maxId = 100000;
    int maxCapacity = 1000;
    size_t maxNote = 2000;
};

// Returns "" if the timeline is acceptable, otherwise "Step N: <reason>".
inline string validateTimeline(const Timeline& tl, const StepLimits& lim = StepLimits()) {
    if (tl.steps.size() > lim.maxSteps) return "The plugin produced " + to_string(tl.steps.size()) + " steps, more than the limit of " + to_string(lim.maxSteps) + ".";
    auto checkDelta = [&](const Delta& d) -> string {
        if (d.container < 0 || d.container > lim.maxContainer) return "container " + to_string(d.container) + " is out of range";
        if (d.target_id > lim.maxId) return "target_id is too large";
        switch (d.kind) {
        case Delta::Kind::ValueChange: case Delta::Kind::NodeCreate: case Delta::Kind::NodeDelete: case Delta::Kind::Label:
            if (d.target_id < 0) return "this delta needs target_id >= 0";
            break;
        case Delta::Kind::RoleChange:
            if (d.target_id < 0) return "RoleChange needs target_id >= 0";
            if (d.new_value < 0 || d.new_value > static_cast<int>(Role::GroupB)) return "RoleChange has an unknown role value " + to_string(d.new_value);
            break;
        case Delta::Kind::EdgeAdd:
            if (d.target_id < 0 || d.edge_to < 0) return "EdgeAdd needs target_id >= 0 and edge_to >= 0";
            break;
        case Delta::Kind::EdgeRemove:
            if (d.target_id < 0) return "EdgeRemove needs target_id >= 0";
            break;
        case Delta::Kind::Resize:
            if (d.new_value < 0 || d.new_value > lim.maxCapacity) return "Resize to " + to_string(d.new_value) + " is out of range";
            break;
        case Delta::Kind::PointerSet:
            if (d.text.empty()) return "PointerSet needs a label";
            break;
        case Delta::Kind::Clear: break;
        default: return "unknown delta kind";
        }
        int s = static_cast<int>(d.slot);
        if (s < 0 || s > static_cast<int>(Slot::Prev)) return "unknown edge slot";
        return "";
    };
    for (size_t i = 0; i < tl.steps.size(); i++) {
        const Step& s = tl.steps[i];
        string at = "Step " + to_string(i + 1) + ": ";
        if (static_cast<int>(s.op) < 0 || static_cast<int>(s.op) > static_cast<int>(OpType::Iterate)) return at + "unknown operation type";
        if (std::isnan(s.progress) || (s.progress != -1.0f && (s.progress < 0.0f || s.progress > 1.0f))) return at + "progress must be -1 (automatic) or between 0 and 1";
        if (s.note.size() > lim.maxNote) return at + "the explanation text is longer than " + to_string(lim.maxNote) + " characters";
        for (const Delta& d : s.deltas) { string e = checkDelta(d); if (!e.empty()) return at + e; }
        for (const RoleTag& r : s.roles) {
            if (r.target_id < 0 || r.target_id > lim.maxId) return at + "a role tag points at an invalid entity (" + to_string(r.target_id) + ")";
            if (r.container < 0 || r.container > lim.maxContainer) return at + "a role tag uses an invalid container";
            if (static_cast<int>(r.role) < 0 || static_cast<int>(r.role) > static_cast<int>(Role::GroupB)) return at + "a role tag has an unknown role";
        }
    }
    for (const Delta& d : tl.initial) { string e = checkDelta(d); if (!e.empty()) return "Initial state: " + e; }
    return "";
}

#endif
