// Tests for src/core/trees (M4): Binary Tree, BST and AVL, including all four rotations.
#include <algorithm>
#include <random>
#include <set>

#include "../TestFramework.h"
#include "../../src/core/trees/AVLTree.h"
#include "../../src/core/trees/BinaryTree.h"

static Command cmd(const string& op, int a = 0) { Command c; c.op = op; c.a = a; return c; }

// Rebuilds in-order keys from the recorded world (root pointer + Left/Right edges).
static void worldInorder(const Container* c, int id, vector<int>& out) {
    if (id < 0) return;
    int l = -1, r = -1;
    auto it = c->edges.find(id);
    if (it != c->edges.end()) for (const auto& e : it->second) { if (e.slot == Slot::Left) l = e.to; if (e.slot == Slot::Right) r = e.to; }
    worldInorder(c, l, out);
    out.push_back(c->nodes.at(id).value);
    worldInorder(c, r, out);
}
static vector<int> worldKeys(const WorldState& w) {
    vector<int> r;
    const Container* c = w.find(0);
    if (!c) return r;
    auto it = c->pointers.find("root");
    if (it != c->pointers.end()) worldInorder(c, it->second, r);
    return r;
}
static int countOp(const Timeline& tl, OpType op) { int n = 0; for (const auto& s : tl.steps) if (s.op == op) n++; return n; }

template <class T>
static Timeline runCmds(T& algo, vector<int> initial, vector<Command> cmds, size_t visibleFrom = 0) {
    Input in; in.values = initial; in.commands = cmds; in.visibleFrom = visibleFrom;
    return generate(algo, in);
}

TEST_CASE(bst_insert_search_min_max_traversals) {
    BstAlgorithm algo;
    Timeline tl = runCmds(algo, {50, 30, 70, 20, 40, 60, 80}, {cmd("insert", 35)});
    tf::verifyTimeline(tl);
    CHECK(algo.inorderKeys() == (vector<int>{20, 30, 35, 40, 50, 60, 70, 80}));
    CHECK(algo.preorderKeys() == (vector<int>{50, 30, 20, 40, 35, 70, 60, 80}));
    CHECK(worldKeys(tf::finalState(tl)) == algo.inorderKeys());
    CHECK(countOp(tl, OpType::EdgeAdd) == 1);

    for (const char* t : {"inorder", "preorder", "postorder"}) {
        BstAlgorithm a2;
        Timeline tt = runCmds(a2, {50, 30, 70}, {cmd(t)});
        tf::verifyTimeline(tt);
        ExecutionEngine e; e.load(tt); e.toEnd();
        CHECK_EQ(e.stats().nodesVisited, 3L);
    }
    BstAlgorithm a3;
    Timeline inord = runCmds(a3, {50, 30, 70, 20}, {cmd("inorder")});
    CHECK(inord.steps.back().note.find("20, 30, 50, 70") != string::npos);
}

TEST_CASE(bst_delete_all_three_cases) {
    BstAlgorithm algo;
    // leaf
    Timeline t1 = runCmds(algo, {50, 30, 70, 20, 40, 60, 80}, {cmd("delete", 20)});
    tf::verifyTimeline(t1);
    CHECK(algo.inorderKeys() == (vector<int>{30, 40, 50, 60, 70, 80}));
    // one child
    BstAlgorithm a2;
    Timeline t2 = runCmds(a2, {50, 30, 70, 20}, {cmd("delete", 30)});
    CHECK(a2.inorderKeys() == (vector<int>{20, 50, 70}));
    CHECK(worldKeys(tf::finalState(t2)) == a2.inorderKeys());
    // two children (root)
    BstAlgorithm a3;
    Timeline t3 = runCmds(a3, {50, 30, 70, 20, 40, 60, 80}, {cmd("delete", 50)});
    tf::verifyTimeline(t3);
    CHECK(a3.inorderKeys() == (vector<int>{20, 30, 40, 60, 70, 80}));
    CHECK(worldKeys(tf::finalState(t3)) == a3.inorderKeys());
    CHECK(a3.isBst());
    // root with a single child, and the last node
    BstAlgorithm a4;
    Timeline t4 = runCmds(a4, {10, 5}, {cmd("delete", 10), cmd("delete", 5)});
    tf::verifyTimeline(t4);
    CHECK(a4.size() == 0);
    CHECK(worldKeys(tf::finalState(t4)).empty());
}

TEST_CASE(bst_errors_empty_missing_duplicate) {
    BstAlgorithm a1;
    CHECK(!runCmds(a1, {}, {cmd("delete", 1)}).error.empty());
    BstAlgorithm a2;
    CHECK(!runCmds(a2, {}, {cmd("search", 1)}).error.empty());
    BstAlgorithm a3;
    CHECK(!runCmds(a3, {}, {cmd("min")}).error.empty());
    BstAlgorithm a4;
    CHECK(!runCmds(a4, {}, {cmd("inorder")}).error.empty());
    BstAlgorithm a5;
    Timeline dup = runCmds(a5, {5}, {cmd("insert", 5)});
    CHECK(dup.error.empty());
    CHECK(a5.size() == 1);
    BstAlgorithm a6;
    Timeline miss = runCmds(a6, {5}, {cmd("search", 9)});
    CHECK(miss.steps.back().note.find("not in the tree") != string::npos);
    BstAlgorithm a7;
    Timeline del = runCmds(a7, {5}, {cmd("delete", 9)});
    CHECK(del.error.empty());
    CHECK(a7.size() == 1);
}

TEST_CASE(bst_min_max) {
    BstAlgorithm algo;
    Timeline mn = runCmds(algo, {50, 30, 70, 20, 80}, {cmd("min")});
    CHECK(mn.steps.back().note == "Minimum is 20.");
    BstAlgorithm a2;
    Timeline mx = runCmds(a2, {50, 30, 70, 20, 80}, {cmd("max")});
    CHECK(mx.steps.back().note == "Maximum is 80.");
}

TEST_CASE(bst_random_operations_match_std_set) {
    mt19937 rng(7);
    for (int round = 0; round < 25; round++) {
        BstAlgorithm algo;
        Input in;
        set<int> ref;
        for (int i = 0; i < 25; i++) {
            int v = (int)(rng() % 30);
            bool del = rng() % 3 == 0 && !ref.empty();     // deleting from an empty tree is a (tested) error
            in.commands.push_back(cmd(del ? "delete" : "insert", v));
            if (del) ref.erase(v); else ref.insert(v);
        }
        in.visibleFrom = in.commands.size() - 1;
        Timeline tl = generate(algo, in);
        CHECK(tl.error.empty());
        CHECK(algo.inorderKeys() == vector<int>(ref.begin(), ref.end()));
        CHECK(worldKeys(tf::finalState(tl)) == algo.inorderKeys());
    }
}

TEST_CASE(avl_four_rotation_cases) {
    struct Case { vector<int> keys; const char* name; vector<int> pre; };
    vector<Case> cases = {{{30, 20, 10}, "LL", {20, 10, 30}}, {{10, 20, 30}, "RR", {20, 10, 30}},
                          {{30, 10, 20}, "LR", {20, 10, 30}}, {{10, 30, 20}, "RL", {20, 10, 30}}};
    for (const auto& c : cases) {
        AvlAlgorithm algo;
        vector<Command> cmds;
        for (int k : c.keys) cmds.push_back(cmd("insert", k));
        Timeline tl = runCmds(algo, {}, cmds, cmds.size() - 1);
        tf::verifyTimeline(tl);
        CHECK(algo.preorderKeys() == c.pre);
        CHECK(algo.isAvlBalanced());
        CHECK(countOp(tl, OpType::Rotate) >= 1);
        bool named = false;
        string tag = string(c.name[0] == 'L' ? "Left" : "Right") + "-" + (c.name[1] == 'L' ? "Left" : "Right");
        for (const auto& s : tl.steps) if (s.note.find(tag) != string::npos) named = true;
        CHECK(named);
        if (c.name[0] != c.name[1]) CHECK_EQ(countOp(tl, OpType::Rotate), 2);   // double rotation
        CHECK(worldKeys(tf::finalState(tl)) == algo.inorderKeys());
    }
}

TEST_CASE(avl_stays_balanced_under_random_inserts_and_deletes) {
    mt19937 rng(11);
    for (int round = 0; round < 40; round++) {
        AvlAlgorithm algo;
        Input in;
        set<int> ref;
        int n = 5 + (int)(rng() % 40);
        for (int i = 0; i < n; i++) {
            int v = (int)(rng() % 50);
            bool del = i > 8 && rng() % 3 == 0 && !ref.empty();
            in.commands.push_back(cmd(del ? "delete" : "insert", v));
            if (del) ref.erase(v); else ref.insert(v);
        }
        in.visibleFrom = in.commands.size() - 1;
        Timeline tl = generate(algo, in);
        CHECK(tl.error.empty());
        CHECK(algo.isAvlBalanced());
        CHECK(algo.inorderKeys() == vector<int>(ref.begin(), ref.end()));
        CHECK(worldKeys(tf::finalState(tl)) == algo.inorderKeys());
    }
}

TEST_CASE(avl_sorted_input_has_logarithmic_height_and_labels) {
    AvlAlgorithm algo;
    Input in;
    for (int i = 1; i <= 31; i++) in.values.push_back(i);
    in.commands.push_back(cmd("search", 17));
    Timeline tl = generate(algo, in);
    CHECK(algo.height() == 5);                            // perfect tree of 31 nodes
    WorldState w = tf::finalState(tl);
    int labelled = 0;
    for (const auto& n : w.find(0)->nodes) if (n.second.label.rfind("bf=", 0) == 0) labelled++;
    CHECK_EQ(labelled, 31);
}

TEST_CASE(binary_tree_level_order_insert_and_delete) {
    BinaryTreeAlgorithm algo;
    Timeline tl = runCmds(algo, {1, 2, 3, 4, 5}, {cmd("insert", 6)});
    tf::verifyTimeline(tl);
    CHECK(algo.levelOrderKeys() == (vector<int>{1, 2, 3, 4, 5, 6}));
    CHECK(algo.inorderKeys() == (vector<int>{4, 2, 5, 1, 6, 3}));
    CHECK(worldKeys(tf::finalState(tl)) == algo.inorderKeys());

    BinaryTreeAlgorithm a2;
    Timeline del = runCmds(a2, {1, 2, 3, 4, 5, 6}, {cmd("delete", 2)});
    tf::verifyTimeline(del);
    CHECK(a2.levelOrderKeys() == (vector<int>{1, 6, 3, 4, 5}));          // deepest-rightmost (6) replaces 2
    CHECK(worldKeys(tf::finalState(del)) == a2.inorderKeys());

    BinaryTreeAlgorithm a3;
    Timeline one = runCmds(a3, {7}, {cmd("delete", 7)});
    CHECK(a3.size() == 0);
    CHECK(worldKeys(tf::finalState(one)).empty());
}

TEST_CASE(binary_tree_search_min_max_and_errors) {
    BinaryTreeAlgorithm a;
    Timeline s = runCmds(a, {4, 9, 2}, {cmd("search", 2)});
    bool found = false;
    for (const auto& st : s.steps) for (const auto& r : st.roles) if (r.role == Role::Found) found = true;
    CHECK(found);
    BinaryTreeAlgorithm b;
    CHECK(runCmds(b, {4, 9, 2}, {cmd("max")}).steps.back().note == "Maximum is 9.");
    BinaryTreeAlgorithm c;
    CHECK(runCmds(c, {4, 9, 2}, {cmd("min")}).steps.back().note == "Minimum is 2.");
    BinaryTreeAlgorithm d;
    CHECK(!runCmds(d, {}, {cmd("delete", 1)}).error.empty());
}

TEST_CASE(trees_history_commands_run_silently) {
    BstAlgorithm algo;
    vector<Command> cmds = {cmd("insert", 5), cmd("insert", 3), cmd("search", 3)};
    Timeline tl = runCmds(algo, {}, cmds, 2);
    WorldState init; init.apply(tl.initial);
    CHECK(worldKeys(init) == (vector<int>{3, 5}));
    CHECK(tl.steps.front().op == OpType::Compare);       // only the search is visible
}
