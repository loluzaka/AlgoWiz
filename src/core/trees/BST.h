#ifndef BST_H
#define BST_H

// Binary Search Tree: insert, search, delete (leaf / one child / two children), minimum,
// maximum and the traversals from TreeBase. AVL derives from this class and only adds the
// afterInsert / afterDelete rebalancing hooks.
//
// DECISION: duplicate keys are rejected with an explanation (the usual lab convention).

#include "TreeCommon.h"

using namespace std;

class BstAlgorithm : public TreeBase {
public:
    BstAlgorithm() {
        info_.id = "bst"; info_.name = "Binary Search Tree"; info_.category = "Trees";
        info_.best = "O(log n)"; info_.average = "O(log n)"; info_.worst = "O(n)"; info_.space = "O(n)";
        info_.description = "Left subtree < node < right subtree. Search, insert and delete follow one root-to-leaf path. Skewed input degrades it to O(n).";
        info_.pseudocode = {"insert(k): start at root", "  if k < node: go left, else if k > node: go right", "  when the child slot is empty: attach new node",
                            "search(k): same walk, stop when k == node", "delete(k): leaf -> remove; one child -> bypass; two children ->",
                            "  copy in-order successor, delete the successor", "min: follow left children", "max: follow right children",
                            "in-order: L, root, R (sorted order)", "pre-order: root, L, R", "post-order: L, R, root"};
        info_.operationPseudocodeLines = {{"insert", {0, 1, 2}}, {"search", {3}}, {"delete", {4, 5}},
                                          {"min", {6}}, {"max", {7}}, {"inorder", {8}},
                                          {"preorder", {9}}, {"postorder", {10}}};
        CppSourceBlock visitSource = {"BST.h::extreme", {
            "TNode* nxt = wantMax",
            "    ? c->r : c->l;"
        }, 0};
        CppSourceBlock stopSource = {"BST.h::extreme", {
            "if (!nxt) break;"
        }, 0};
        info_.cppSourceBlocks[6][OpType::VisitNode] = visitSource;
        info_.cppSourceBlocks[6][OpType::Highlight] = stopSource;
        info_.cppSourceBlocks[7] = info_.cppSourceBlocks[6];
    }

protected:
    virtual void afterInsert(TNode*) {}
    virtual void afterDelete(TNode*) {}

    void insertKey(int key) override {
        if (!root_) {
            TNode* n = makeNode(key);
            setRoot(n, "The tree was empty: " + to_string(key) + " becomes the root.");
            afterInsert(n);
            return;
        }
        path_.clear();
        TNode* c = root_;
        while (true) {
            if (key == c->key) {
                ev(OpType::Compare, {ptrD("cur", c)}, pathRoles(c, Role::Error), to_string(key) + " = " + k(c) + ": duplicate keys are not allowed, nothing is inserted.", 1);
                path_.clear();
                return;
            }
            bool left = key < c->key;
            ev(OpType::Compare, {ptrD("cur", c)}, pathRoles(c, Role::Compared),
               to_string(key) + (left ? " < " : " > ") + k(c) + ", so go to the " + (left ? "left" : "right") + " subtree.", 1);
            TNode* nxt = left ? c->l : c->r;
            path_.push_back(c->id);
            if (!nxt) {
                TNode* n = makeNode(key);
                link(c, n, left, "The " + string(left ? "left" : "right") + " slot of " + k(c) + " is empty: insert " + to_string(key) + " as its " + (left ? "left" : "right") + " child.", 2);
                afterInsert(n);
                return;
            }
            c = nxt;
        }
    }

    void searchKey(int key) override {
        requireNonEmpty("search");
        path_.clear();
        TNode* c = root_;
        while (c) {
            bool eq = key == c->key;
            ev(OpType::Compare, {ptrD("cur", c)}, pathRoles(c, Role::Compared),
               eq ? to_string(key) + " = " + k(c) + ": found." : to_string(key) + (key < c->key ? " < " : " > ") + k(c) + ", so the search moves to the " + (key < c->key ? "left" : "right") + " subtree.", 3);
            if (eq) { ev(OpType::Highlight, {ptrD("cur", nullptr)}, pathRoles(c, Role::Found), "Found " + to_string(key) + "."); path_.clear(); return; }
            path_.push_back(c->id);
            c = key < c->key ? c->l : c->r;
        }
        finish(to_string(key) + " is not in the tree (we fell off a leaf).");
    }

    void extreme(bool wantMax) override {
        requireNonEmpty(wantMax ? "find the maximum" : "find the minimum");
        path_.clear();
        TNode* c = root_;
        while (true) {
            TNode* nxt = wantMax ? c->r : c->l;
            ev(OpType::VisitNode, {ptrD("cur", c)}, pathRoles(c, Role::Current),
               nxt ? string("Node ") + k(c) + " has a " + (wantMax ? "right" : "left") + " child, keep going." : string("Node ") + k(c) + " has no " + (wantMax ? "right" : "left") + " child.", wantMax ? 7 : 6);
            if (!nxt) break;
            path_.push_back(c->id);
            c = nxt;
        }
        ev(OpType::Highlight, {ptrD("cur", nullptr)}, pathRoles(c, Role::Found), string(wantMax ? "Maximum" : "Minimum") + " is " + k(c) + ".", wantMax ? 7 : 6);
        path_.clear();
    }

    void deleteKey(int key) override {
        requireNonEmpty("delete");
        path_.clear();
        TNode* c = root_;
        while (c && c->key != key) {
            bool left = key < c->key;
            ev(OpType::Compare, {ptrD("cur", c)}, pathRoles(c, Role::Compared), to_string(key) + (left ? " < " : " > ") + k(c) + ", so go " + (left ? "left." : "right."), 4);
            path_.push_back(c->id);
            c = left ? c->l : c->r;
        }
        if (!c) { finish(to_string(key) + " is not in the tree, nothing to delete."); return; }
        ev(OpType::Highlight, {ptrD("cur", c)}, pathRoles(c, Role::Candidate), "Found " + to_string(key) + ": this node will be deleted.", 4);

        TNode* target = c;
        if (c->l && c->r) {
            ev(OpType::Highlight, {}, pathRoles(c, Role::Candidate), "Node " + k(c) + " has two children: replace it with its in-order successor (smallest key of the right subtree).", 5);
            path_.push_back(c->id);
            TNode* s = c->r;
            while (true) {
                ev(OpType::VisitNode, {ptrD("cur", s)}, pathRoles(s, Role::Current), s->l ? "Successor search: " + k(s) + " has a left child, keep going." : "Successor found: " + k(s) + " has no left child.", 5);
                if (!s->l) break;
                path_.push_back(s->id);
                s = s->l;
            }
            c->key = s->key;
            ev(OpType::Assign, {Delta::value(c->id, s->key), ptrD("cur", nullptr)}, {rt(c, Role::Moved), rt(s, Role::Candidate)}, "Copy the successor key " + k(s) + " into the node that held " + to_string(key) + ".", 5);
            target = s;
        }
        TNode* child = target->l ? target->l : target->r;
        TNode* p = target->p;
        string what = child ? "one child" : "no children";
        vector<Delta> d;
        if (!p) {
            root_ = child;
            if (child) child->p = nullptr;
            d.push_back(ptrD("root", child));
        } else {
            bool wasLeft = p->l == target;
            (wasLeft ? p->l : p->r) = child;
            if (child) { child->p = p; d.push_back(Delta::edgeAdd(p->id, child->id, wasLeft ? Slot::Left : Slot::Right)); }
            else d.push_back(Delta::edgeRemove(p->id, target->id, wasLeft ? Slot::Left : Slot::Right));
        }
        vector<RoleTag> roles = {rt(target, Role::Candidate)};
        if (p) roles.push_back(rt(p, Role::Path));
        if (child) roles.push_back(rt(child, Role::Path));
        ev(OpType::EdgeRemove, d, roles, "Node " + k(target) + " has " + what + ": " + (child ? "its parent now points to its child." : "detach it from its parent."), 4);
        ev(OpType::NodeDelete, {Delta::destroy(target->id)}, {}, "Free the removed node.", 4);
        freeNode(target);
        path_.clear();
        afterDelete(p);
        finish("Deleted " + to_string(key) + ".");
    }
};

inline void registerBst(AlgorithmCatalog& cat) {
    CatalogEntry e;
    BstAlgorithm probe;
    e.id = probe.info().id; e.category = "Trees"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Tree; e.ops = TreeBase::ops();
    e.make = [] { return unique_ptr<Algorithm>(new BstAlgorithm()); };
    cat.add(e);
}

#endif
