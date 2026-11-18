#ifndef BINARYTREE_H
#define BINARYTREE_H

// Plain binary tree (no ordering rule).
//
// DECISION (the specification does not define insertion for a plain binary tree):
//   * insert  = LEVEL-ORDER insertion: the new node becomes the first free child slot found
//               by a breadth-first scan (left before right).
//   * delete  = replace the target with the DEEPEST, RIGHTMOST node, then remove that node.
//   * search / min / max = breadth-first scan, because there is no ordering to exploit.
// This keeps the tree complete (as balanced as possible) and is the textbook approach.

#include <queue>

#include "TreeCommon.h"

using namespace std;

class BinaryTreeAlgorithm : public TreeBase {
public:
    BinaryTreeAlgorithm() {
        info_.id = "binary_tree"; info_.name = "Binary Tree"; info_.category = "Trees";
        info_.best = "O(1) insert"; info_.average = "O(n)"; info_.worst = "O(n)"; info_.space = "O(n)";
        info_.description = "Each node has up to two children. Insertion here is level-order (first free slot), so the tree stays complete.";
        info_.pseudocode = {"insert: BFS from the root", "  first node with a free child slot gets the new node",
                            "search: BFS; stop when the key is found", "min: BFS and keep the smallest key", "max: BFS and keep the largest key",
                            "delete: copy deepest-rightmost key into target", "  remove the deepest-rightmost node",
                            "in-order: L, root, R", "pre-order: root, L, R", "post-order: L, R, root"};
        info_.operationPseudocodeLines = {{"insert", {0, 1}}, {"search", {2}}, {"min", {3}}, {"max", {4}},
                                          {"delete", {5, 6}}, {"inorder", {7}}, {"preorder", {8}}, {"postorder", {9}}};
        CppSourceBlock compareSource = {"BinaryTree.h::extreme", {
            "bool better = wantMax",
            "    ? c->key > best->key",
            "    : c->key < best->key;"
        }, 0};
        CppSourceBlock resultSource = {"BinaryTree.h::extreme", {
            "ev(OpType::Highlight,",
            "   {ptrD(\"cur\", nullptr)},",
            "   {rt(best, Role::Found)},",
            "   string(wantMax ? \"Maximum\" : \"Minimum\") +",
            "       \" is \" + k(best) + \".\",",
            "   wantMax ? 4 : 3);"
        }, 0};
        info_.cppSourceBlocks[3][OpType::Compare] = compareSource;
        info_.cppSourceBlocks[3][OpType::Highlight] = resultSource;
        info_.cppSourceBlocks[4] = info_.cppSourceBlocks[3];
    }
    vector<int> levelOrderKeys() const {
        vector<int> r; queue<TNode*> q;
        if (root_) q.push(root_);
        while (!q.empty()) { TNode* c = q.front(); q.pop(); r.push_back(c->key); if (c->l) q.push(c->l); if (c->r) q.push(c->r); }
        return r;
    }

protected:
    int traversalCodeLine(int order) const override { return 7 + order; }

    void insertKey(int key) override {
        if (!root_) { TNode* n = makeNode(key); setRoot(n, "The tree was empty: the new node becomes the root."); return; }
        queue<TNode*> q; q.push(root_);
        while (!q.empty()) {
            TNode* c = q.front(); q.pop();
            ev(OpType::VisitNode, {ptrD("cur", c)}, {rt(c, Role::Current)}, "Level-order scan: does " + k(c) + " have a free child slot?", 0);
            if (!c->l) { TNode* n = makeNode(key); link(c, n, true, "First free slot: " + to_string(key) + " becomes the LEFT child of " + k(c) + "."); return; }
            if (!c->r) { TNode* n = makeNode(key); link(c, n, false, "First free slot: " + to_string(key) + " becomes the RIGHT child of " + k(c) + "."); return; }
            q.push(c->l); q.push(c->r);
        }
    }

    void searchKey(int key) override {
        requireNonEmpty("search");
        queue<TNode*> q; q.push(root_);
        while (!q.empty()) {
            TNode* c = q.front(); q.pop();
            bool eq = c->key == key;
            ev(OpType::Compare, {ptrD("cur", c)}, {rt(c, Role::Compared)}, "Compare " + k(c) + " with " + to_string(key) + (eq ? ": found." : ": different."), 2);
            if (eq) { ev(OpType::Highlight, {ptrD("cur", nullptr)}, {rt(c, Role::Found)}, "Found " + to_string(key) + "."); return; }
            if (c->l) q.push(c->l);
            if (c->r) q.push(c->r);
        }
        finish(to_string(key) + " is not in the tree.");
    }

    void extreme(bool wantMax) override {
        requireNonEmpty(wantMax ? "find the maximum" : "find the minimum");
        TNode* best = root_;
        queue<TNode*> q; q.push(root_);
        while (!q.empty()) {
            TNode* c = q.front(); q.pop();
            bool better = wantMax ? c->key > best->key : c->key < best->key;
            ev(OpType::Compare, {ptrD("cur", c)}, {rt(c, Role::Compared), rt(best, Role::Candidate)},
               "Compare " + k(c) + " with the best so far (" + k(best) + ")" + (better ? ": new best." : "."), wantMax ? 4 : 3);
            if (better) best = c;
            if (c->l) q.push(c->l);
            if (c->r) q.push(c->r);
        }
        ev(OpType::Highlight, {ptrD("cur", nullptr)}, {rt(best, Role::Found)}, string(wantMax ? "Maximum" : "Minimum") + " is " + k(best) + ".", wantMax ? 4 : 3);
    }

    void deleteKey(int key) override {
        requireNonEmpty("delete");
        TNode* target = nullptr; TNode* last = nullptr;
        queue<TNode*> q; q.push(root_);
        while (!q.empty()) {
            TNode* c = q.front(); q.pop();
            last = c;
            if (!target) {
                bool eq = c->key == key;
                ev(OpType::Compare, {ptrD("cur", c)}, {rt(c, Role::Compared)}, "Compare " + k(c) + " with " + to_string(key) + (eq ? ": this is the node to delete." : ": different."), 5);
                if (eq) target = c;
            }
            if (c->l) q.push(c->l);
            if (c->r) q.push(c->r);
        }
        if (!target) { finish(to_string(key) + " is not in the tree, nothing to delete."); return; }
        ev(OpType::Highlight, {ptrD("cur", nullptr)}, {rt(target, Role::Candidate), rt(last, Role::Pivot)},
           "The deepest, rightmost node is " + k(last) + ". Its key will replace " + k(target) + ".", 5);
        if (target != last) {
            target->key = last->key;
            ev(OpType::Assign, {Delta::value(target->id, last->key)}, {rt(target, Role::Moved)}, "Copy " + k(last) + " into the target node.", 5);
        }
        TNode* p = last->p;
        if (!p) { ev(OpType::NodeDelete, {Delta::destroy(last->id), ptrD("root", nullptr)}, {}, "The only node is removed; the tree is empty.", 6); root_ = nullptr; freeNode(last); return; }
        bool wasLeft = p->l == last;
        (wasLeft ? p->l : p->r) = nullptr;
        ev(OpType::EdgeRemove, {Delta::edgeRemove(p->id, last->id, wasLeft ? Slot::Left : Slot::Right)}, {rt(p, Role::Path)}, "Detach the deepest node from its parent " + k(p) + ".", 6);
        ev(OpType::NodeDelete, {Delta::destroy(last->id)}, {}, "Free the detached node.", 6);
        freeNode(last);
        finish("Deleted " + to_string(key) + ".");
    }
};

inline void registerBinaryTree(AlgorithmCatalog& cat) {
    CatalogEntry e;
    BinaryTreeAlgorithm probe;
    e.id = probe.info().id; e.category = "Trees"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Tree; e.ops = TreeBase::ops();
    e.make = [] { return unique_ptr<Algorithm>(new BinaryTreeAlgorithm()); };
    cat.add(e);
}

#endif
