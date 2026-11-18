#ifndef RACECONTROLLER_H
#define RACECONTROLLER_H

// Race Mode logic: two recorded Timelines are played side by side by ONE controller.
//
//   RaceMode::Ticks    every tick moves BOTH algorithms one step; the one with fewer steps
//                      finishes first (a "race" in the everyday sense).
//   RaceMode::Progress both are kept at the same FRACTION DONE (Step::progress), so unequal step
//                      counts stay visually aligned (Bubble step 50 != Quick step 50).
//
// The controller owns two ExecutionEngines, never draws, and has no Qt or terminal code.
// compareTimelines() builds the table of section 28 (Algorithm Comparison Mode).

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "../ExecutionEngine.h"

using namespace std;

enum class RaceMode { Ticks, Progress };

struct ComparisonRow {
    string name;
    size_t steps = 0;
    Statistics stats;        // totals at the end of the run
    double runtimeMs = 0;    // time spent RECORDING the steps (not a precise benchmark)
};

inline vector<ComparisonRow> compareTimelines(const vector<Timeline>& timelines, const vector<string>& names) {
    vector<ComparisonRow> rows;
    for (size_t i = 0; i < timelines.size(); i++) {
        ExecutionEngine e;
        e.load(timelines[i]);
        ComparisonRow r;
        r.name = i < names.size() ? names[i] : timelines[i].algorithm;
        r.steps = e.total();
        r.stats = e.finalStats();
        r.runtimeMs = timelines[i].runtimeMs;
        rows.push_back(r);
    }
    return rows;
}

inline string comparisonTable(const vector<ComparisonRow>& rows) {
    ostringstream o;
    size_t w = 9;
    for (const auto& r : rows) w = max(w, r.name.size());
    auto line = [&]() { o << "+" << string(w + 2, '-') << "+--------+-------------+-------+-------------+----------+\n"; };
    line();
    o << "| " << left << setw((int)w) << "Algorithm" << " | Steps  | Comparisons | Swaps | Assignments | Rec. ms  |\n";
    line();
    for (const auto& r : rows) {
        o << "| " << left << setw((int)w) << r.name << " | " << setw(6) << r.steps << " | " << setw(11) << r.stats.comparisons
          << " | " << setw(5) << r.stats.swaps << " | " << setw(11) << r.stats.assignments << " | " << fixed << setprecision(3) << setw(8) << r.runtimeMs << " |\n";
    }
    line();
    return o.str();
}

class RaceController {
public:
    void load(Timeline a, Timeline b, string nameA, string nameB) {
        nameA_ = std::move(nameA); nameB_ = std::move(nameB);
        a_.load(std::move(a)); b_.load(std::move(b));
        time_ = 0; ticks_ = 0;
    }
    void setMode(RaceMode m) { mode_ = m; reset(); }
    RaceMode mode() const { return mode_; }

    ExecutionEngine& paneA() { return a_; }
    ExecutionEngine& paneB() { return b_; }
    const ExecutionEngine& paneA() const { return a_; }
    const ExecutionEngine& paneB() const { return b_; }
    const string& nameA() const { return nameA_; }
    const string& nameB() const { return nameB_; }
    size_t ticks() const { return ticks_; }
    bool finished() const { return a_.atEnd() && b_.atEnd(); }
    double fractionA() const { return a_.total() ? (a_.atEnd() ? 1.0 : a_.progress()) : 1.0; }
    double fractionB() const { return b_.total() ? (b_.atEnd() ? 1.0 : b_.progress()) : 1.0; }

    void reset() { a_.reset(); b_.reset(); time_ = 0; ticks_ = 0; }

    // Advances the race by one tick. Returns false when both panes were already finished.
    bool tick() {
        if (finished()) return false;
        ticks_++;
        if (mode_ == RaceMode::Ticks) { a_.next(); b_.next(); return true; }
        // Progress mode: move to the earliest progress value that either pane reaches next.
        double next = 2.0;
        if (!a_.atEnd()) next = min(next, (double)a_.nextStep()->progress);
        if (!b_.atEnd()) next = min(next, (double)b_.nextStep()->progress);
        time_ = next;
        syncTo(a_, next); syncTo(b_, next);
        return true;
    }
    // Rewinds the race by one tick (pointer rewind of both engines).
    bool back() {
        if (ticks_ == 0) return false;
        size_t target = ticks_ - 1;
        reset();
        for (size_t i = 0; i < target; i++) tick();
        return true;
    }

    // Which algorithm needed fewer steps: -1 = A, 1 = B, 0 = tie.
    int fewerSteps() const { return a_.total() == b_.total() ? 0 : (a_.total() < b_.total() ? -1 : 1); }

    vector<ComparisonRow> summary() const {
        vector<ComparisonRow> rows(2);
        const ExecutionEngine* es[2] = {&a_, &b_};
        const string* ns[2] = {&nameA_, &nameB_};
        for (int i = 0; i < 2; i++) {
            rows[i].name = *ns[i]; rows[i].steps = es[i]->total();
            rows[i].stats = es[i]->finalStats(); rows[i].runtimeMs = es[i]->timeline().runtimeMs;
        }
        return rows;
    }

private:
    static void syncTo(ExecutionEngine& e, double t) {
        while (!e.atEnd() && e.nextStep()->progress <= t + 1e-9) e.next();
    }
    ExecutionEngine a_, b_;
    string nameA_, nameB_;
    RaceMode mode_ = RaceMode::Ticks;
    double time_ = 0;
    size_t ticks_ = 0;
};

#endif
