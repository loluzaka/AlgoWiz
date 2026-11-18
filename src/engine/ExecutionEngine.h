#ifndef EXECUTIONENGINE_H
#define EXECUTIONENGINE_H

#include <functional>
#include <vector>

#include "Statistics.h"
#include "Step.h"
#include "WorldState.h"

using namespace std;

// Plays a recorded Timeline. `position()` is the number of steps already applied:
// 0 = initial state, total() = finished. Going backwards is a POINTER REWIND:
// restore the nearest keyframe and replay deltas forward (no inverse deltas).
class ExecutionEngine {
public:
    static const size_t KEYFRAME_EVERY = 16;

    void load(Timeline tl);
    bool next();
    bool previous();
    void reset();
    void jumpTo(size_t position);       // clamps to [0, total()]
    void toEnd() { jumpTo(total()); }

    size_t position() const { return pos_; }
    size_t total() const { return tl_.steps.size(); }
    bool   atStart() const { return pos_ == 0; }
    bool   atEnd() const { return pos_ == total(); }
    bool   loaded() const { return loaded_; }

    const WorldState& state() const { return cur_; }
    WorldState stateAt(size_t position) const;           // rebuilds from a keyframe
    const Step* currentStep() const { return pos_ == 0 ? nullptr : &tl_.steps[pos_ - 1]; }
    const Step* nextStep() const { return pos_ >= total() ? nullptr : &tl_.steps[pos_]; }
    const Statistics& stats() const { return stats_[pos_]; }
    const Statistics& finalStats() const { return stats_.back(); }
    float progress() const;                              // progress of the current step, 0 at start
    const Timeline& timeline() const { return tl_; }

    // Replaces Qt signals: called after every change of position.
    void setListener(function<void()> f) { listener_ = std::move(f); }

private:
    void notify() { if (listener_) listener_(); }
    Timeline tl_;
    WorldState cur_;
    size_t pos_ = 0;
    bool loaded_ = false;
    vector<WorldState> keyframes_;     // keyframes_[k] = state after k*KEYFRAME_EVERY steps
    vector<Statistics> stats_;         // stats_[p] = statistics after p steps
    function<void()> listener_;
};

#endif
