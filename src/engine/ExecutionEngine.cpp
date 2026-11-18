#include "ExecutionEngine.h"

void ExecutionEngine::load(Timeline tl) {
    tl_ = std::move(tl);
    loaded_ = true;
    keyframes_.clear();
    stats_.clear();

    WorldState w;
    w.apply(tl_.initial);
    keyframes_.push_back(w);
    Statistics st;
    stats_.push_back(st);
    for (size_t i = 0; i < tl_.steps.size(); i++) {
        for (const Delta& d : tl_.steps[i].deltas) w.apply(d);
        st.add(tl_.steps[i]);
        stats_.push_back(st);
        if ((i + 1) % KEYFRAME_EVERY == 0) keyframes_.push_back(w);
    }
    cur_ = keyframes_[0];
    pos_ = 0;
    notify();
}

bool ExecutionEngine::next() {
    if (!loaded_ || atEnd()) return false;
    cur_.apply(tl_.steps[pos_].deltas);
    pos_++;
    notify();
    return true;
}

WorldState ExecutionEngine::stateAt(size_t position) const {
    if (position > total()) position = total();
    size_t k = position / KEYFRAME_EVERY;
    WorldState w = keyframes_[k];
    for (size_t i = k * KEYFRAME_EVERY; i < position; i++) w.apply(tl_.steps[i].deltas);
    return w;
}

void ExecutionEngine::jumpTo(size_t position) {
    if (!loaded_) return;
    if (position > total()) position = total();
    if (position == pos_) return;
    if (position == pos_ + 1) { next(); return; }
    cur_ = stateAt(position);
    pos_ = position;
    notify();
}

bool ExecutionEngine::previous() {
    if (!loaded_ || atStart()) return false;
    jumpTo(pos_ - 1);
    return true;
}

void ExecutionEngine::reset() { jumpTo(0); }

float ExecutionEngine::progress() const {
    const Step* s = currentStep();
    return s ? s->progress : 0.f;
}
