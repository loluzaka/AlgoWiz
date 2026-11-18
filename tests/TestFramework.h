#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

// A tiny self-registering test framework (no external dependency, works offline).
//   TEST_CASE(sorting_bubble_empty) { CHECK(...); CHECK_EQ(a, b); }
// plus helpers that every module test uses to validate recorded Timelines.

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../src/engine/Algorithm.h"
#include "../src/engine/ExecutionEngine.h"

namespace tf {
struct TestCase { std::string name; void (*fn)(); };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& checks() { static int c = 0; return c; }
inline int& failures() { static int f = 0; return f; }
struct Registrar { Registrar(const char* n, void (*f)()) { registry().push_back({n, f}); } };
}

#define TEST_CASE(name) static void name(); static tf::Registrar reg_##name(#name, name); static void name()

#define CHECK(cond) do { ++tf::checks(); if (!(cond)) { ++tf::failures(); \
    std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  CHECK(" #cond ")\n"; } } while (0)

#define CHECK_EQ(a, b) do { ++tf::checks(); auto _va = (a); auto _vb = (b); if (!(_va == _vb)) { ++tf::failures(); \
    std::ostringstream _o; _o << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #a " == " #b "  (" << _va << " vs " << _vb << ")\n"; \
    std::cerr << _o.str(); } } while (0)

// ---------------------------------------------------------------- helpers
namespace tf {

inline std::string join(const std::vector<int>& v) {
    std::ostringstream o; o << "[";
    for (size_t i = 0; i < v.size(); i++) o << (i ? "," : "") << v[i];
    o << "]"; return o.str();
}
inline std::string join(const std::vector<std::string>& v) {
    std::ostringstream o; o << "[";
    for (size_t i = 0; i < v.size(); i++) o << (i ? "," : "") << v[i];
    o << "]"; return o.str();
}

// Values of container `c` in slot order 0..capacity-1 (empty slots skipped).
inline std::vector<int> intsOf(const WorldState& w, int c = 0) {
    std::vector<int> r;
    const Container* k = w.find(c);
    if (!k) return r;
    for (const auto& n : k->nodes) r.push_back(n.second.value);
    return r;
}
inline std::vector<std::string> textsOf(const WorldState& w, int c = 0) {
    std::vector<std::string> r;
    const Container* k = w.find(c);
    if (!k) return r;
    for (const auto& n : k->nodes) r.push_back(n.second.text);
    return r;
}

// Replays the whole timeline and checks the engine contract:
//  * seq strictly increasing, progress inside [0,1] and monotonic, last step == 1
//  * stateAt(p) (pointer rewind) equals the state reached by stepping forward
//  * previous() after next() returns to the identical state
inline void verifyTimeline(const Timeline& tl) {
    CHECK(tl.error.empty());
    float prev = 0.f;
    for (size_t i = 0; i < tl.steps.size(); i++) {
        CHECK(tl.steps[i].seq == i);
        CHECK(tl.steps[i].progress >= 0.f && tl.steps[i].progress <= 1.f);
        CHECK(tl.steps[i].progress >= prev);
        prev = tl.steps[i].progress;
    }
    if (!tl.steps.empty()) CHECK(tl.steps.back().progress == 1.f);
    ExecutionEngine e;
    e.load(tl);
    std::vector<WorldState> forward;
    forward.push_back(e.state());
    while (e.next()) forward.push_back(e.state());
    CHECK(forward.size() == tl.steps.size() + 1);
    bool allEqual = true;
    for (size_t p = 0; p < forward.size(); p++)
        if (!(e.stateAt(p) == forward[p])) allEqual = false;
    CHECK(allEqual);
    bool backOk = true;
    while (e.previous()) if (!(e.state() == forward[e.position()])) backOk = false;
    CHECK(backOk);
    CHECK(e.position() == 0);
}

// Runs an algorithm through generate() and returns the final world state.
inline WorldState finalState(const Timeline& tl) {
    ExecutionEngine e;
    e.load(tl);
    e.toEnd();
    return e.state();
}

inline std::vector<std::vector<int>> sampleArrays() {
    return {{}, {7}, {2, 1}, {1, 2}, {1, 2, 3, 4, 5}, {5, 4, 3, 2, 1}, {3, 3, 1, 3, 2, 2},
            {8, 3, 5, 1, 9, 2}, {-3, 7, 0, -3, 12, 5, 1}, {50, 20, 90, 10, 40, 30, 80}};
}

} // namespace tf

#endif
