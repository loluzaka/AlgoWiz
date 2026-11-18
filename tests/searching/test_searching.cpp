// Tests for src/core/searching (M3): target present / absent, empty, one, two, duplicates,
// the sortedness rules of Binary / Interpolation search and Median search.
#include <algorithm>

#include "../TestFramework.h"
#include "../../src/core/searching/Searches.h"

static Timeline runSearch(SearchKind k, vector<int> v, int target) {
    SearchAlgorithm a(k);
    Input in; in.values = v; in.hasTarget = true; in.target = target;
    return generate(a, in);
}
static bool foundMark(const Timeline& tl, int idx) {
    ExecutionEngine e; e.load(tl); e.toEnd();
    return e.state().markOf(0, idx) == Role::Found;
}
static bool anyFound(const Timeline& tl) {
    ExecutionEngine e; e.load(tl); e.toEnd();
    const Container* c = e.state().find(0);
    if (!c) return false;
    for (const auto& m : c->marks) if (m.second == Role::Found) return true;
    return false;
}

TEST_CASE(searching_linear_present_absent) {
    Timeline tl = runSearch(SearchKind::Linear, {4, 9, 2, 7}, 2);
    tf::verifyTimeline(tl);
    CHECK(foundMark(tl, 2));
    Timeline miss = runSearch(SearchKind::Linear, {4, 9, 2, 7}, 5);
    tf::verifyTimeline(miss);
    CHECK(!anyFound(miss));
    CHECK(miss.steps.back().roles.size() == 4);          // NotFound on every cell
}

TEST_CASE(searching_linear_counts_comparisons) {
    ExecutionEngine e; e.load(runSearch(SearchKind::Linear, {4, 9, 2, 7}, 2)); e.toEnd();
    CHECK_EQ(e.stats().comparisons, 3L);
}

TEST_CASE(searching_empty_array_is_rejected_gracefully) {
    for (SearchKind k : {SearchKind::Linear, SearchKind::Binary, SearchKind::Interpolation, SearchKind::Median}) {
        Timeline tl = runSearch(k, {}, 1);
        CHECK(!tl.error.empty());
    }
}

TEST_CASE(searching_binary_requires_sorted) {
    CHECK(!runSearch(SearchKind::Binary, {3, 1, 2}, 2).error.empty());
    CHECK(!runSearch(SearchKind::Interpolation, {3, 1, 2}, 2).error.empty());
}

TEST_CASE(searching_binary_all_targets_on_sorted_arrays) {
    for (const auto& arr : tf::sampleArrays()) {
        vector<int> s = arr; sort(s.begin(), s.end());
        if (s.empty()) continue;
        for (int t = -5; t <= 55; t += 1) {
            for (SearchKind k : {SearchKind::Binary, SearchKind::Interpolation, SearchKind::Linear}) {
                Timeline tl = runSearch(k, s, t);
                CHECK(tl.error.empty());
                bool present = binary_search(s.begin(), s.end(), t);
                CHECK_EQ(anyFound(tl), present);
            }
        }
    }
}

TEST_CASE(searching_binary_halves_the_range) {
    vector<int> v; for (int i = 0; i < 16; i++) v.push_back(i * 3);
    Timeline tl = runSearch(SearchKind::Binary, v, 45);
    tf::verifyTimeline(tl);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK(e.stats().comparisons <= 5);                 // log2(16) + 1
}

TEST_CASE(searching_binary_marks_eliminated_ranges) {
    Timeline tl = runSearch(SearchKind::Binary, {1, 3, 5, 7, 9, 11}, 11);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK(e.state().markOf(0, 0) == Role::Eliminated);
}

TEST_CASE(searching_interpolation_handles_equal_values) {
    Timeline tl = runSearch(SearchKind::Interpolation, {5, 5, 5, 5}, 5);
    CHECK(tl.error.empty());
    CHECK(anyFound(tl));
    Timeline miss = runSearch(SearchKind::Interpolation, {5, 5, 5, 5}, 6);
    CHECK(miss.error.empty());
    CHECK(!anyFound(miss));
}

TEST_CASE(searching_median_finds_lower_median) {
    for (const auto& arr : tf::sampleArrays()) {
        if (arr.empty()) continue;
        SearchAlgorithm a(SearchKind::Median);
        Input in; in.values = arr;
        Timeline tl = generate(a, in);
        tf::verifyTimeline(tl);
        ExecutionEngine e; e.load(tl); e.toEnd();
        vector<int> sorted = arr; sort(sorted.begin(), sorted.end());
        int expect = sorted[(sorted.size() - 1) / 2];
        const Container* c = e.state().find(0);
        int got = 0; bool seen = false;
        for (const auto& m : c->marks) if (m.second == Role::Found) { got = c->nodes.at(m.first).value; seen = true; }
        CHECK(seen);
        CHECK_EQ(got, expect);
    }
}

TEST_CASE(searching_needs_target) {
    SearchAlgorithm a(SearchKind::Linear);
    Input in; in.values = {1, 2};
    CHECK(!generate(a, in).error.empty());
}
