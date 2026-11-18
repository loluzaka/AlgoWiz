// Tests for src/core/graphs (M4): edit operations, directed / undirected modes, BFS and DFS orders.
#include <algorithm>

#include "../TestFramework.h"
#include "../../src/core/graphs/Graph.h"

static Command cmd(const string& op, int a = 0, int b = 0, int c = 1) { Command x; x.op = op; x.a = a; x.b = b; x.c = c; return x; }

// Triples u v w -> Input::values
static Input sampleGraph(bool directed) {
    Input in; in.directed = directed; in.vertexCount = 6;
    //  A-B, A-C, B-D, C-D, C-E, D-F
    in.values = {0, 1, 1, 0, 2, 1, 1, 3, 1, 2, 3, 1, 2, 4, 1, 3, 5, 1};
    return in;
}

TEST_CASE(graph_initial_build_is_silent) {
    GraphAlgorithm algo;
    Input in = sampleGraph(false);
    in.commands.push_back(cmd("bfs", 0));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    WorldState init; init.apply(tl.initial);
    CHECK_EQ(init.find(0)->nodes.size(), (size_t)6);
    CHECK_EQ(algo.edgeCount(), 6);
    CHECK_EQ(init.find(0)->pointers.at("directed"), 0);
}

TEST_CASE(graph_bfs_order_and_queue_stats) {
    GraphAlgorithm algo;
    Input in = sampleGraph(false);
    in.commands.push_back(cmd("bfs", 0));
    Timeline tl = generate(algo, in);
    CHECK(algo.lastOrder() == (vector<int>{0, 1, 2, 3, 4, 5}));
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().nodesVisited, 6L);
    CHECK_EQ(e.stats().edgesExplored, 12L);               // every undirected edge is seen from both ends
    CHECK_EQ(e.stats().inserts, 6L);                      // 6 enqueues
    CHECK_EQ(e.stats().deletes, 6L);                      // 6 dequeues
    CHECK(tl.steps.back().note.find("A -> B -> C -> D -> E -> F") != string::npos);
}

TEST_CASE(graph_dfs_order_and_recursion_stack) {
    GraphAlgorithm algo;
    Input in = sampleGraph(false);
    in.commands.push_back(cmd("dfs", 0));
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(algo.lastOrder() == (vector<int>{0, 1, 3, 2, 4, 5}));
    WorldState w = tf::finalState(tl);
    CHECK(w.find(1) == nullptr || w.find(1)->nodes.empty());            // stack is empty again
}

TEST_CASE(graph_directed_bfs_follows_arrows) {
    GraphAlgorithm algo;
    Input in; in.directed = true; in.vertexCount = 4;
    in.values = {0, 1, 1, 1, 2, 1, 3, 0, 1};
    in.commands.push_back(cmd("bfs", 0));
    Timeline tl = generate(algo, in);
    CHECK(algo.lastOrder() == (vector<int>{0, 1, 2}));              // D (3) is unreachable
    CHECK_EQ(algo.edgeCount(), 3);
}

TEST_CASE(graph_edit_operations) {
    GraphAlgorithm algo;
    Input in; in.vertexCount = 3;
    in.commands = {cmd("add_edge", 0, 1, 4), cmd("add_edge", 1, 2, 7), cmd("set_weight", 0, 1, 9), cmd("add_vertex"),
                   cmd("add_edge", 2, 3, 2), cmd("delete_edge", 1, 2), cmd("delete_vertex", 0)};
    in.visibleFrom = in.commands.size() - 1;
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(algo.vertices() == (vector<int>{1, 2, 3}));
    CHECK(algo.hasEdge(2, 3) && algo.hasEdge(3, 2));
    CHECK(!algo.hasEdge(1, 2));
    CHECK(!algo.hasEdge(0, 1));
    WorldState w = tf::finalState(tl);
    CHECK_EQ(w.find(0)->nodes.size(), (size_t)3);
    CHECK(w.find(0)->nodes.count(0) == 0);
}

TEST_CASE(graph_weights_update_both_directions_when_undirected) {
    GraphAlgorithm algo;
    Input in; in.vertexCount = 2;
    in.commands = {cmd("add_edge", 0, 1, 4), cmd("set_weight", 0, 1, 9)};
    in.visibleFrom = 1;
    generate(algo, in);
    CHECK_EQ(algo.weight(0, 1), 9);
    CHECK_EQ(algo.weight(1, 0), 9);
}

TEST_CASE(graph_invalid_operations_report_errors) {
    auto bad = [](vector<Command> cmds, int vertices = 2) {
        GraphAlgorithm algo;
        Input in; in.vertexCount = vertices; in.commands = cmds;
        return !generate(algo, in).error.empty();
    };
    CHECK(bad({cmd("add_edge", 0, 0)}));                         // self loop
    CHECK(bad({cmd("add_edge", 0, 5)}));                         // missing vertex
    CHECK(bad({cmd("delete_edge", 0, 1)}));                      // no such edge
    CHECK(bad({cmd("set_weight", 0, 1, 3)}));
    CHECK(bad({cmd("bfs", 9)}));
    CHECK(bad({cmd("delete_vertex", 7)}));
    CHECK(bad({cmd("add_edge", 0, 1), cmd("add_edge", 0, 1)}));  // duplicate
    CHECK(bad({cmd("add_vertex")}, 10));                         // more than 10 vertices
    GraphAlgorithm algo;
    Input toomany; toomany.vertexCount = 11;
    CHECK(!generate(algo, toomany).error.empty());
}

TEST_CASE(graph_empty_and_single_vertex_traversals) {
    GraphAlgorithm algo;
    Input in; in.vertexCount = 1; in.commands = {cmd("bfs", 0), cmd("dfs", 0)};
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    CHECK(algo.lastOrder() == (vector<int>{0}));
    GraphAlgorithm a2;
    Input none; none.commands = {cmd("bfs", 0)};
    CHECK(!generate(a2, none).error.empty());                    // no vertices
}

TEST_CASE(graph_cleanup_between_visible_commands) {
    GraphAlgorithm algo;
    Input in = sampleGraph(false);
    in.commands = {cmd("bfs", 0), cmd("dfs", 0)};
    Timeline tl = generate(algo, in);
    tf::verifyTimeline(tl);
    bool cleaned = false;
    for (const auto& s : tl.steps) if (s.op == OpType::Unmark) cleaned = true;
    CHECK(cleaned);
}
