// Tests for src/core/lists (M3): singly / doubly / circular lists and the array structure.
#include <deque>

#include "../TestFramework.h"
#include "../../src/core/lists/ArrayStructure.h"
#include "../../src/core/lists/LinkedList.h"

using Type = LinkedListAlgorithm::Type;

// Follows the Next edges that were recorded in the world, starting at the HEAD pointer.
static vector<int> worldOrder(const WorldState& w, int limit = 100) {
    vector<int> r;
    const Container* c = w.find(0);
    if (!c) return r;
    auto hp = c->pointers.find("HEAD");
    if (hp == c->pointers.end()) return r;
    int cur = hp->second;
    for (int i = 0; i < limit && cur >= 0; i++) {
        r.push_back(c->nodes.at(cur).value);
        int nxt = -1;
        auto it = c->edges.find(cur);
        if (it != c->edges.end()) for (const auto& e : it->second) if (e.slot == Slot::Next) nxt = e.to;
        if (nxt == hp->second) break;          // circular: back at HEAD
        cur = nxt;
    }
    return r;
}

struct Session {
    LinkedListAlgorithm algo;
    Input in;
    explicit Session(Type t) : algo(t) {}
    Timeline run(const string& op, int a = 0, int b = 0) {
        Command c; c.op = op; c.a = a; c.b = b;
        in.commands.push_back(c);
        in.visibleFrom = in.commands.size() - 1;
        Timeline tl = generate(algo, in);
        if (!tl.error.empty()) in.commands.pop_back();
        return tl;
    }
};

TEST_CASE(lists_insert_and_delete_all_types_match_model) {
    for (Type t : {Type::Singly, Type::Doubly, Type::Circular}) {
        Session s(t);
        deque<int> model;
        auto check = [&](const Timeline& tl) {
            CHECK(tl.error.empty());
            tf::verifyTimeline(tl);
            WorldState w = tf::finalState(tl);
            CHECK(worldOrder(w) == vector<int>(model.begin(), model.end()));
            CHECK(s.algo.values() == vector<int>(model.begin(), model.end()));
        };
        model.push_back(10); check(s.run("insert_end", 10));
        model.push_back(20); check(s.run("insert_end", 20));
        model.push_front(5); check(s.run("insert_begin", 5));
        model.insert(model.begin() + 2, 15); check(s.run("insert_pos", 2, 15));
        model.push_front(1); check(s.run("insert_pos", 0, 1));
        model.push_back(99); check(s.run("insert_pos", 5, 99));
        model.erase(model.begin() + 3); check(s.run("delete_pos", 3));
        model.pop_front(); check(s.run("delete_begin"));
        model.pop_back(); check(s.run("delete_end"));
        model.erase(model.begin() + 1); check(s.run("delete_pos", 1));
        while (!model.empty()) { model.pop_back(); check(s.run("delete_end")); }
    }
}

TEST_CASE(lists_invalid_operations_report_errors) {
    for (Type t : {Type::Singly, Type::Doubly, Type::Circular}) {
        Session s(t);
        CHECK(!s.run("delete_begin").error.empty());
        CHECK(!s.run("delete_end").error.empty());
        CHECK(!s.run("search", 1).error.empty());
        CHECK(!s.run("traverse").error.empty());
        CHECK(!s.run("insert_pos", 3, 1).error.empty());
        CHECK(s.run("insert_end", 1).error.empty());
        CHECK(!s.run("insert_pos", -1, 1).error.empty());
        CHECK(!s.run("delete_pos", 1).error.empty());
    }
}

TEST_CASE(lists_search_found_and_missing) {
    Session s(Type::Doubly);
    for (int v : {4, 8, 15}) s.run("insert_end", v);
    Timeline hit = s.run("search", 8);
    tf::verifyTimeline(hit);
    bool found = false;
    for (const auto& st : hit.steps) for (const auto& r : st.roles) if (r.role == Role::Found) found = true;
    CHECK(found);
    Timeline miss = s.run("search", 99);
    tf::verifyTimeline(miss);
    CHECK(miss.steps.back().note.find("not in the list") != string::npos);
}

TEST_CASE(lists_traverse_visits_every_node_in_order) {
    Session s(Type::Circular);
    for (int v : {1, 2, 3}) s.run("insert_end", v);
    Timeline tl = s.run("traverse");
    tf::verifyTimeline(tl);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().nodesVisited, 3L);
    CHECK(tl.steps.back().note.find("1 -> 2 -> 3") != string::npos);
}

TEST_CASE(lists_doubly_keeps_prev_pointers_consistent) {
    Session s(Type::Doubly);
    for (int v : {1, 2, 3, 4}) s.run("insert_end", v);
    Timeline tl = s.run("delete_pos", 2);
    WorldState w = tf::finalState(tl);
    const Container* c = w.find(0);
    // every Next edge a->b must have a matching Prev edge b->a
    for (const auto& kv : c->edges)
        for (const auto& e : kv.second)
            if (e.slot == Slot::Next) {
                bool back = false;
                auto it = c->edges.find(e.to);
                if (it != c->edges.end()) for (const auto& q : it->second) if (q.slot == Slot::Prev && q.to == kv.first) back = true;
                CHECK(back);
            }
}

TEST_CASE(lists_initial_values_are_built_silently) {
    LinkedListAlgorithm algo(Type::Singly);
    Input in; in.values = {7, 8, 9};
    Command c; c.op = "insert_end"; c.a = 10; in.commands.push_back(c);
    Timeline tl = generate(algo, in);
    WorldState w;
    w.apply(tl.initial);
    CHECK(worldOrder(w) == (vector<int>{7, 8, 9}));
    CHECK(worldOrder(tf::finalState(tl)) == (vector<int>{7, 8, 9, 10}));
}

TEST_CASE(arrays_insert_delete_shift_elements) {
    ArrayStructure algo;
    Input in; in.values = {1, 2, 4, 5};
    Command c; c.op = "insert"; c.a = 2; c.b = 3; in.commands.push_back(c);
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(tf::intsOf(tf::finalState(tl)) == (vector<int>{1, 2, 3, 4, 5}));
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().assignments, 2L);                 // two cells shifted

    Input in2; in2.values = {1, 2, 3, 4, 5};
    Command d; d.op = "delete"; d.a = 1; in2.commands.push_back(d);
    ArrayStructure algo2;
    Timeline t2 = generate(algo2, in2);
    CHECK(tf::intsOf(tf::finalState(t2)) == (vector<int>{1, 3, 4, 5}));
}

TEST_CASE(arrays_invalid_positions_and_empty) {
    ArrayStructure algo;
    Input in; Command c; c.op = "delete"; c.a = 0; in.commands.push_back(c);
    CHECK(!generate(algo, in).error.empty());
    Input in2; in2.values = {1}; Command d; d.op = "insert"; d.a = 5; d.b = 1; in2.commands.push_back(d);
    ArrayStructure algo2;
    CHECK(!generate(algo2, in2).error.empty());
    Input in3; in3.values = {1}; Command u; u.op = "update"; u.a = 3; u.b = 1; in3.commands.push_back(u);
    ArrayStructure algo3;
    CHECK(!generate(algo3, in3).error.empty());
}
