// Tests for src/core/heaps (M4): min heap, max heap, priority queue, build heap, heapify.
#include <algorithm>
#include <functional>
#include <queue>
#include <random>

#include "../TestFramework.h"
#include "../../src/core/heaps/Heap.h"

static Command cmd(const string& op, int a = 0, const string& s = "") { Command c; c.op = op; c.a = a; c.s = s; return c; }

TEST_CASE(heap_min_and_max_insert_keep_property) {
    for (HeapKind k : {HeapKind::Min, HeapKind::Max}) {
        HeapAlgorithm algo(k);
        Input in;
        for (int v : {5, 3, 8, 1, 9, 2, 7}) in.commands.push_back(cmd("insert", v));
        in.visibleFrom = in.commands.size() - 1;
        Timeline tl = generate(algo, in);
        tf::verifyTimeline(tl);
        CHECK(algo.isHeap());
        CHECK_EQ(algo.contents()[0], k == HeapKind::Min ? 1 : 9);
        CHECK(tf::intsOf(tf::finalState(tl)) == algo.contents());
    }
}

TEST_CASE(heap_extract_returns_sorted_order) {
    for (HeapKind k : {HeapKind::Min, HeapKind::Max, HeapKind::PriorityQueue}) {
        HeapAlgorithm algo(k);
        Input in; in.values = {5, 3, 8, 1, 9, 2, 7};
        vector<int> expect = in.values;
        sort(expect.begin(), expect.end());
        if (k == HeapKind::Max) reverse(expect.begin(), expect.end());
        for (size_t i = 0; i < expect.size(); i++) in.commands.push_back(cmd("extract"));
        in.visibleFrom = 0;
        Timeline tl = generate(algo, in);
        tf::verifyTimeline(tl);
        // the sequence of extracted roots is announced in the notes: "Extract the root X."
        vector<int> got;
        for (const auto& s : tl.steps) {
            size_t p = s.note.find("Extract the root ");
            if (p != string::npos) got.push_back(stoi(s.note.substr(p + 17)));
        }
        CHECK(got == expect);
        CHECK(algo.contents().empty());
    }
}

TEST_CASE(heap_random_operations_match_std_priority_queue) {
    mt19937 rng(3);
    for (int round = 0; round < 30; round++) {
        HeapAlgorithm algo(HeapKind::Min);
        priority_queue<int, vector<int>, greater<int>> ref;
        Input in;
        for (int i = 0; i < 20; i++) {
            if (rng() % 3 == 0 && !ref.empty()) { in.commands.push_back(cmd("extract")); ref.pop(); }
            else if (ref.size() < 15) { int v = (int)(rng() % 40); in.commands.push_back(cmd("insert", v)); ref.push(v); }
        }
        in.visibleFrom = in.commands.empty() ? 0 : in.commands.size() - 1;
        Timeline tl = generate(algo, in);
        CHECK(tl.error.empty());
        CHECK(algo.isHeap());
        vector<int> rest;
        while (!ref.empty()) { rest.push_back(ref.top()); ref.pop(); }
        vector<int> got = algo.contents();
        sort(got.begin(), got.end());
        CHECK(got == rest);
    }
}

TEST_CASE(heap_build_heap_floyd) {
    for (HeapKind k : {HeapKind::Min, HeapKind::Max}) {
        HeapAlgorithm algo(k);
        Input in; in.commands.push_back(cmd("build_heap", 0, "9 4 7 1 8 2 6 3 5"));
        Timeline tl = generate(algo, in);
        tf::verifyTimeline(tl);
        CHECK(algo.isHeap());
        CHECK_EQ(algo.contents().size(), (size_t)9);
        ExecutionEngine e; e.load(tl); e.toEnd();
        CHECK(e.stats().heapifies >= 4);                 // n/2 = 4 sift-downs
    }
}

TEST_CASE(heap_heapify_index_and_errors) {
    HeapAlgorithm algo(HeapKind::Min);
    Input in; in.values = {1, 3, 2};
    in.commands.push_back(cmd("heapify", 0));
    Timeline tl = generate(algo, in);
    CHECK(tl.error.empty());
    CHECK(algo.isHeap());

    HeapAlgorithm a2(HeapKind::Min);
    Input bad; bad.commands.push_back(cmd("heapify", 5));
    CHECK(!generate(a2, bad).error.empty());
    HeapAlgorithm a3(HeapKind::Min);
    Input u; u.commands.push_back(cmd("extract"));
    CHECK(!generate(a3, u).error.empty());
    HeapAlgorithm a4(HeapKind::Min);
    Input p; p.commands.push_back(cmd("peek"));
    CHECK(!generate(a4, p).error.empty());
    HeapAlgorithm a5(HeapKind::Min);
    Input b; b.commands.push_back(cmd("build_heap", 0, "   "));
    CHECK(!generate(a5, b).error.empty());
}

TEST_CASE(heap_overflow_at_fifteen) {
    HeapAlgorithm algo(HeapKind::Max);
    Input in;
    for (int i = 0; i < 16; i++) in.commands.push_back(cmd("insert", i));
    in.visibleFrom = 15;
    Timeline tl = generate(algo, in);
    CHECK(!tl.error.empty());
}

TEST_CASE(heap_peek_does_not_change_state) {
    HeapAlgorithm algo(HeapKind::Min);
    Input in; in.values = {4, 2, 6};
    in.commands.push_back(cmd("peek"));
    Timeline tl = generate(algo, in);
    WorldState init; init.apply(tl.initial);
    CHECK(tf::intsOf(init) == tf::intsOf(tf::finalState(tl)));
}
