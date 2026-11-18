// Tests for src/core/linear and src/core/hashing (M1): stack, queue, circular queue,
// hash tables (linear / quadratic / chaining).
#include <algorithm>
#include <deque>

#include "../TestFramework.h"
#include "../../src/core/hashing/HashTable.h"
#include "../../src/core/linear/Queue.h"
#include "../../src/core/linear/Stack.h"

static Command cmd(const string& op, int a = 0, int b = 0) { Command c; c.op = op; c.a = a; c.b = b; return c; }

static bool anyNoteContains(const Timeline& tl, const string& text) {
    for (const auto& s : tl.steps) if (s.note.find(text) != string::npos) return true;
    return false;
}

TEST_CASE(stack_push_pop_peek_and_pointer) {
    StackAlgorithm algo;
    Input in; in.capacity = 4;
    for (int v : {1, 2, 3}) in.commands.push_back(cmd("push", v));
    in.commands.push_back(cmd("pop"));
    in.commands.push_back(cmd("peek"));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    WorldState w = tf::finalState(tl);
    CHECK(tf::intsOf(w) == (vector<int>{1, 2}));
    CHECK_EQ(w.find(0)->pointers.at("TOP"), 1);
    CHECK(algo.contents() == (vector<int>{1, 2}));
}

TEST_CASE(stack_overflow_and_underflow_are_shown_not_crashed) {
    StackAlgorithm algo;
    Input in; in.capacity = 2;
    for (int v : {1, 2, 3}) in.commands.push_back(cmd("push", v));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(anyNoteContains(tl, "OVERFLOW"));
    CHECK(tf::intsOf(tf::finalState(tl)) == (vector<int>{1, 2}));

    StackAlgorithm algo2;
    Input in2; in2.capacity = 2; in2.commands.push_back(cmd("pop")); in2.commands.push_back(cmd("peek"));
    Timeline t2 = generate(algo2, in2);
    CHECK(anyNoteContains(t2, "UNDERFLOW"));
    CHECK(tf::intsOf(tf::finalState(t2)).empty());
}

TEST_CASE(stack_is_empty_is_full_and_validation) {
    StackAlgorithm algo;
    Input in; in.capacity = 1;
    in.commands.push_back(cmd("is_empty")); in.commands.push_back(cmd("push", 5)); in.commands.push_back(cmd("is_full"));
    Timeline tl = generate(algo, in);
    CHECK(anyNoteContains(tl, "isEmpty() = true"));
    CHECK(anyNoteContains(tl, "isFull() = true"));
    Input bad; bad.capacity = 0;
    StackAlgorithm algo2;
    CHECK(!generate(algo2, bad).error.empty());
    Input bad2; bad2.capacity = 2; bad2.values = {1, 2, 3};
    StackAlgorithm algo3;
    CHECK(!generate(algo3, bad2).error.empty());
}

TEST_CASE(stack_initial_values_and_silent_replay) {
    StackAlgorithm algo;
    Input in; in.capacity = 5; in.values = {9, 8};
    in.commands.push_back(cmd("push", 7)); in.commands.push_back(cmd("pop")); in.commands.push_back(cmd("push", 6));
    in.visibleFrom = 2;                                  // first two commands are history
    Timeline tl = generate(algo, in);
    WorldState init; init.apply(tl.initial);
    CHECK(tf::intsOf(init) == (vector<int>{9, 8}));
    CHECK(tf::intsOf(tf::finalState(tl)) == (vector<int>{9, 8, 6}));
}

TEST_CASE(queue_linear_fifo_and_flaw) {
    QueueAlgorithm algo(false);
    Input in; in.capacity = 3;
    for (int v : {1, 2, 3}) in.commands.push_back(cmd("enqueue", v));
    in.commands.push_back(cmd("dequeue"));
    in.commands.push_back(cmd("enqueue", 4));            // linear queue: REAR is at the end -> overflow
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(anyNoteContains(tl, "OVERFLOW"));
    CHECK(anyNoteContains(tl, "cannot reuse"));
    CHECK(algo.contents() == (vector<int>{2, 3}));
}

TEST_CASE(queue_circular_wraps_around) {
    QueueAlgorithm algo(true);
    Input in; in.capacity = 3;
    for (int v : {1, 2, 3}) in.commands.push_back(cmd("enqueue", v));
    in.commands.push_back(cmd("dequeue"));
    in.commands.push_back(cmd("enqueue", 4));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(!anyNoteContains(tl, "OVERFLOW"));
    CHECK(algo.contents() == (vector<int>{2, 3, 4}));
    WorldState w = tf::finalState(tl);
    CHECK_EQ(w.find(0)->pointers.at("REAR"), 0);          // wrapped to index 0
    CHECK_EQ(w.find(0)->pointers.at("FRONT"), 1);
}

TEST_CASE(queue_underflow_peek_and_empty_reset) {
    QueueAlgorithm algo(false);
    Input in; in.capacity = 2;
    in.commands.push_back(cmd("dequeue")); in.commands.push_back(cmd("front")); in.commands.push_back(cmd("rear"));
    in.commands.push_back(cmd("enqueue", 1)); in.commands.push_back(cmd("dequeue")); in.commands.push_back(cmd("enqueue", 2));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(anyNoteContains(tl, "UNDERFLOW"));
    CHECK(!anyNoteContains(tl, "OVERFLOW"));              // reset after emptying, so the slot is reusable
    CHECK(algo.contents() == (vector<int>{2}));
}

// --------------------------------------------------------------------- hashing
static Timeline hashRun(HashMethod m, int size, vector<int> keys, vector<Command> extra = {}, HashTableAlgorithm** keep = nullptr) {
    static HashTableAlgorithm* last = nullptr;
    delete last;
    last = new HashTableAlgorithm(m);
    Input in; in.tableSize = size;
    for (int k : keys) in.commands.push_back(cmd("insert", k));
    for (auto& c : extra) in.commands.push_back(c);
    Timeline tl = generate(*last, in);
    if (keep) *keep = last;
    return tl;
}

TEST_CASE(hash_linear_probing_example_from_spec) {
    // 21 and 44 collide at index 4 (m = 10 -> but table size is capped at 13, use m = 5: 21%5=1, 44%5=4 ...)
    HashTableAlgorithm* algo = nullptr;
    Timeline tl = hashRun(HashMethod::Linear, 5, {21, 41, 51}, {}, &algo);   // all hash to 1
    tf::verifyTimeline(tl);
    WorldState w = tf::finalState(tl);
    CHECK_EQ(w.find(0)->nodes.at(1).value, 21);
    CHECK_EQ(w.find(0)->nodes.at(2).value, 41);
    CHECK_EQ(w.find(0)->nodes.at(3).value, 51);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().collisions, 3L);                   // 41 collides once, 51 twice
    CHECK_EQ(e.stats().hashCalcs, 3L);
}

TEST_CASE(hash_quadratic_probe_sequence) {
    Timeline tl = hashRun(HashMethod::Quadratic, 7, {7, 14, 21});            // all hash to 0 -> 0, 1, 4
    WorldState w = tf::finalState(tl);
    CHECK_EQ(w.find(0)->nodes.at(0).value, 7);
    CHECK_EQ(w.find(0)->nodes.at(1).value, 14);
    CHECK_EQ(w.find(0)->nodes.at(4).value, 21);
}

TEST_CASE(hash_search_and_delete_with_tombstones) {
    HashTableAlgorithm* algo = nullptr;
    Timeline tl = hashRun(HashMethod::Linear, 5, {1, 6, 11}, {cmd("delete", 6), cmd("search", 11)}, &algo);
    tf::verifyTimeline(tl);
    CHECK(anyNoteContains(tl, "Found 11"));
    CHECK(anyNoteContains(tl, "tombstone"));
    CHECK(algo->keysInTable() == (vector<int>{1, 11}));
}

TEST_CASE(hash_search_missing_and_full_table) {
    Timeline miss = hashRun(HashMethod::Linear, 5, {1}, {cmd("search", 2)});
    CHECK(anyNoteContains(miss, "not in the table"));
    Timeline full = hashRun(HashMethod::Linear, 3, {1, 2, 3, 4});
    CHECK(anyNoteContains(full, "cannot be inserted"));
}

TEST_CASE(hash_duplicates_are_not_inserted) {
    HashTableAlgorithm* algo = nullptr;
    Timeline tl = hashRun(HashMethod::Chaining, 5, {7, 7}, {}, &algo);
    CHECK(algo->keysInTable() == (vector<int>{7}));
    CHECK(anyNoteContains(tl, "already"));
}

TEST_CASE(hash_chaining_builds_and_unlinks_chains) {
    HashTableAlgorithm* algo = nullptr;
    Timeline tl = hashRun(HashMethod::Chaining, 5, {5, 10, 15, 3}, {cmd("delete", 10)}, &algo);
    tf::verifyTimeline(tl);
    CHECK(algo->keysInTable() == (vector<int>{5, 15, 3}));
    WorldState w = tf::finalState(tl);
    // slot 0 -> node(5) -> node(15): follow the Next edges from slot 0
    const Container* c = w.find(0);
    vector<int> chain;
    int cur = 0;
    for (int guard = 0; guard < 10; guard++) {
        auto it = c->edges.find(cur);
        if (it == c->edges.end() || it->second.empty()) break;
        cur = it->second[0].to;
        chain.push_back(c->nodes.at(cur).value);
    }
    CHECK(chain == (vector<int>{5, 15}));
}

TEST_CASE(hash_negative_keys_and_table_size_validation) {
    HashTableAlgorithm* algo = nullptr;
    Timeline tl = hashRun(HashMethod::Linear, 5, {-3}, {}, &algo);
    CHECK(algo->keysInTable() == (vector<int>{-3}));
    CHECK_EQ(tf::finalState(tl).find(0)->nodes.count(2), (size_t)1);       // -3 mod 5 -> 2
    HashTableAlgorithm bad(HashMethod::Linear);
    Input in; in.tableSize = 2;
    CHECK(!generate(bad, in).error.empty());
}
