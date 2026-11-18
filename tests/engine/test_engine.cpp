// Tests for src/engine (M1): WorldState deltas, ExecutionEngine playback / pointer rewind,
// Statistics, Playback clock, Recorder, generate() error isolation.
#include "../TestFramework.h"
#include "../../src/engine/Playback.h"
#include "../../src/engine/Recorder.h"
#include "../../src/engine/Statistics.h"

static Step mk(OpType op, vector<Delta> d = {}, string note = "") {
    Step s; s.op = op; s.deltas = d; s.note = note; return s;
}

TEST_CASE(world_value_create_delete) {
    WorldState w;
    w.apply(Delta::resize(3));
    w.apply(Delta::value(0, 5));
    w.apply(Delta::valueText(1, "hi"));
    CHECK_EQ(w.find(0)->capacity, 3);
    CHECK_EQ(w.find(0)->nodes.at(0).value, 5);
    CHECK(w.find(0)->nodes.at(1).hasText);
    w.apply(Delta::value(1, 9));                       // numeric overwrite clears the text
    CHECK(!w.find(0)->nodes.at(1).hasText);
    w.apply(Delta::destroy(0));
    CHECK_EQ(w.find(0)->nodes.count(0), (size_t)0);
}

TEST_CASE(world_edges_replace_by_slot_and_remove) {
    WorldState w;
    w.apply(Delta::edgeAdd(1, 2, Slot::Left));
    w.apply(Delta::edgeAdd(1, 3, Slot::Left));          // pointer semantics: replaces
    CHECK_EQ(w.find(0)->edges.at(1).size(), (size_t)1);
    CHECK_EQ(w.find(0)->edges.at(1)[0].to, 3);
    w.apply(Delta::edgeAdd(1, 4, Slot::Right));
    CHECK_EQ(w.find(0)->edges.at(1).size(), (size_t)2);
    w.apply(Delta::edgeRemove(1, 3, Slot::Left));
    CHECK_EQ(w.find(0)->edges.at(1).size(), (size_t)1);
    w.apply(Delta::edgeAdd(5, 6, Slot::None, 3));       // graph edge, then weight update
    w.apply(Delta::edgeAdd(5, 6, Slot::None, 8));
    CHECK_EQ(w.find(0)->edges.at(5).size(), (size_t)1);
    CHECK_EQ(w.find(0)->edges.at(5)[0].weight, 8);
}

TEST_CASE(world_node_delete_drops_dangling_edges_and_marks) {
    WorldState w;
    w.apply(Delta::create(1, 10)); w.apply(Delta::create(2, 20));
    w.apply(Delta::edgeAdd(1, 2, Slot::Next));
    w.apply(Delta::mark(2, Role::Visited));
    w.apply(Delta::destroy(2));
    CHECK(w.find(0)->edges.count(1) == 0);
    CHECK(w.markOf(0, 2) == Role::Default);
}

TEST_CASE(world_pointers_labels_marks_clear_resize) {
    WorldState w;
    w.apply(Delta::pointer("i", 3));
    CHECK_EQ(w.find(0)->pointers.at("i"), 3);
    w.apply(Delta::pointer("i", -1));
    CHECK(w.find(0)->pointers.count("i") == 0);
    w.apply(Delta::value(2, 1)); w.apply(Delta::label(2, "bf=0"));
    CHECK_EQ(w.find(0)->nodes.at(2).label, string("bf=0"));
    w.apply(Delta::mark(2, Role::Sorted));
    CHECK(w.markOf(0, 2) == Role::Sorted);
    w.apply(Delta::mark(2, Role::Default));
    CHECK(w.markOf(0, 2) == Role::Default);
    w.apply(Delta::value(7, 1)); w.apply(Delta::resize(4));
    CHECK_EQ(w.find(0)->nodes.count(7), (size_t)0);     // resize drops slots beyond the capacity
    w.apply(Delta::clear());
    CHECK(w.find(0)->nodes.empty());
    w.apply(Delta::value(1, 4, 1));                     // other container
    CHECK(w.find(1) != nullptr && w.find(0)->nodes.empty());
    CHECK(!w.signature().empty());
}

static Timeline numbered(int n) {
    Timeline tl;
    tl.initial = {Delta::resize(1), Delta::value(0, 0)};
    for (int i = 1; i <= n; i++) tl.steps.push_back(mk(OpType::Assign, {Delta::value(0, i)}, "set " + to_string(i)));
    finalizeTimeline(tl);
    return tl;
}

TEST_CASE(engine_next_previous_reset_jump) {
    ExecutionEngine e;
    CHECK(!e.next());                                    // nothing loaded
    e.load(numbered(40));                                // more than two keyframes
    CHECK_EQ(e.total(), (size_t)40);
    CHECK(e.atStart() && e.currentStep() == nullptr);
    for (int i = 1; i <= 40; i++) { CHECK(e.next()); CHECK_EQ(tf::intsOf(e.state())[0], i); }
    CHECK(e.atEnd() && !e.next());
    for (int i = 39; i >= 0; i--) { CHECK(e.previous()); CHECK_EQ(tf::intsOf(e.state())[0], i); }
    CHECK(!e.previous());
    e.jumpTo(27); CHECK_EQ(tf::intsOf(e.state())[0], 27); CHECK_EQ(e.position(), (size_t)27);
    e.jumpTo(5);  CHECK_EQ(tf::intsOf(e.state())[0], 5);
    e.jumpTo(999); CHECK(e.atEnd());
    e.reset(); CHECK(e.atStart() && tf::intsOf(e.state())[0] == 0);
    tf::verifyTimeline(numbered(40));
}

TEST_CASE(engine_listener_fires_on_every_change) {
    ExecutionEngine e;
    int calls = 0;
    e.setListener([&] { calls++; });
    e.load(numbered(3));
    int base = calls;
    e.next(); e.next(); e.previous(); e.reset();
    CHECK_EQ(calls - base, 4);
}

TEST_CASE(engine_statistics_follow_position) {
    Timeline tl;
    tl.steps.push_back(mk(OpType::Compare));
    tl.steps.push_back(mk(OpType::Swap));
    tl.steps.push_back(mk(OpType::Assign));
    tl.steps.push_back(mk(OpType::Rotate));
    tl.steps.push_back(mk(OpType::Collision));
    finalizeTimeline(tl);
    ExecutionEngine e; e.load(tl);
    CHECK_EQ(e.stats().steps, 0L);
    e.toEnd();
    CHECK_EQ(e.stats().comparisons, 1L); CHECK_EQ(e.stats().swaps, 1L); CHECK_EQ(e.stats().assignments, 1L);
    CHECK_EQ(e.stats().rotations, 1L);  CHECK_EQ(e.stats().collisions, 1L);
    CHECK_EQ(e.stats().accesses, 8L);                    // 2 + 4 + 2
    e.jumpTo(2);
    CHECK_EQ(e.stats().swaps, 1L); CHECK_EQ(e.stats().assignments, 0L);
    CHECK_EQ(e.finalStats().steps, 5L);
    CHECK(!e.finalStats().rows().empty());
}

TEST_CASE(timeline_progress_is_filled_and_monotonic) {
    Timeline tl;
    for (int i = 0; i < 4; i++) tl.steps.push_back(mk(OpType::Note));
    tl.steps[1].progress = 0.9f;                         // explicit values are respected, then clamped monotone
    finalizeTimeline(tl);
    CHECK(tl.steps[0].progress == 0.25f);
    CHECK(tl.steps[1].progress == 0.9f);
    CHECK(tl.steps[2].progress == 0.9f);
    CHECK(tl.steps[3].progress == 1.f);
    Timeline one; one.steps.push_back(mk(OpType::Note));
    finalizeTimeline(one);
    CHECK(one.steps[0].progress == 1.f);
}

TEST_CASE(recorder_mute_folds_deltas_into_initial) {
    Timeline tl;
    Recorder rec(tl);
    rec.muted = true;
    rec.emit(OpType::Assign, {Delta::value(0, 1)}, {}, "hidden");
    rec.muted = false;
    rec.emit(OpType::Assign, {Delta::value(0, 2)}, {}, "visible");
    CHECK_EQ(tl.initial.size(), (size_t)1);
    CHECK_EQ(tl.steps.size(), (size_t)1);
}

TEST_CASE(array_trace_methods_emit_matching_steps) {
    Timeline tl;
    Recorder rec(tl);
    ArrayTrace<int> t(rec, {3, 1, 2}, 0);
    t.compare(0, 1, "cmp");
    t.swap(0, 1, "swap");
    t.set(2, 9, "set");
    t.mark({0}, Role::Sorted, "mark");
    t.pointer("i", 1, "ptr");
    t.pointer("i", -1, "ptr off");
    finalizeTimeline(tl);
    tf::verifyTimeline(tl);
    WorldState w = tf::finalState(tl);
    CHECK(tf::intsOf(w) == (vector<int>{1, 3, 9}));
    CHECK(w.markOf(0, 0) == Role::Sorted);
    CHECK(w.find(0)->pointers.empty());
    CHECK(t.data() == (vector<int>{1, 3, 9}));
}

TEST_CASE(playback_clock_speeds_and_skip) {
    Playback p;
    CHECK_EQ(p.advance(10000.0), 0);                     // paused
    p.play();
    CHECK_EQ(p.advance(599), 0);
    CHECK_EQ(p.advance(1), 1);                           // Normal = 600 ms
    p.level = SpeedLevel::Fast;
    CHECK_EQ(p.advance(500), 2);                         // 250 ms
    p.fastForward = true;
    CHECK(p.advance(250) >= 4);                          // 62.5 ms interval
    p.skipAnimation = true;
    CHECK(p.advance(1) > 1000000);
    p.pause();
    CHECK_EQ(p.advance(1), 0);
    p.cycleSpeed();
    CHECK(p.level == SpeedLevel::Turbo);
    p.cycleSpeed();
    CHECK(p.level == SpeedLevel::Slow);
    CHECK(Playback::name(SpeedLevel::Slow) == "Slow");
}

struct ThrowingAlgo : Algorithm {
    AlgorithmInfo i;
    int mode;
    explicit ThrowingAlgo(int m) : mode(m) { i.id = "throwing"; i.name = "Throwing"; }
    const AlgorithmInfo& info() const override { return i; }
    string validate(const Input& in) const override { return in.values.empty() && mode == 0 ? "needs values" : ""; }
    Timeline run(const Input&) override {
        if (mode == 1) throw InvalidOperation("bad user action");
        if (mode == 2) throw runtime_error("boom");
        if (mode == 3) throw 42;
        return Timeline();
    }
};

TEST_CASE(generate_isolates_failures_and_validation) {
    ThrowingAlgo a0(0); CHECK(generate(a0, Input()).error == "needs values");
    ThrowingAlgo a1(1); Input in; in.values = {1};
    CHECK(generate(a1, in).error == "bad user action");
    ThrowingAlgo a2(2); CHECK(generate(a2, in).error.find("boom") != string::npos);
    ThrowingAlgo a3(3); CHECK(!generate(a3, in).error.empty());
    ThrowingAlgo ok(4); Timeline tl = generate(ok, in);
    CHECK(tl.error.empty() && tl.algorithm == "throwing" && tl.steps.empty());
    ExecutionEngine e; e.load(tl);                       // zero-step timeline plays fine
    CHECK(e.atStart() && e.atEnd() && !e.next());
}
