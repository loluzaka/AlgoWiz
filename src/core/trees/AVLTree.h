#ifndef AVLTREE_H
#define AVLTREE_H

// AVL tree = BST + height / balance-factor bookkeeping + the four rotations
// (LL, RR, LR, RL), each rotation shown as its own Rotate step.
// balance factor = height(left) - height(right); the tree rebalances whenever |bf| > 1.

#include <cstdlib>

#include "BST.h"

using namespace std;

class AvlAlgorithm : public BstAlgorithm {
public:
    AvlAlgorithm() {
        info_.id = "avl"; info_.name = "AVL Tree";
        info_.best = "O(log n)"; info_.average = "O(log n)"; info_.worst = "O(log n)"; info_.space = "O(n)";
        info_.description = "A self-balancing BST. After every insert/delete the heights are updated on the way up and any node with |balance factor| > 1 is fixed by rotations.";
        info_.pseudocode = {"insert(k): start at root", "  if k < node: go left, else if k > node: go right", "  when the child slot is empty: attach new node",
                            "search(k): same walk, stop when k == node", "delete(k): leaf -> remove; one child -> bypass; two children ->",
                            "  copy in-order successor, delete the successor", "min: follow left children", "max: follow right children",
                            "in-order: L, root, R (sorted order)", "pre-order: root, L, R", "post-order: L, R, root",
                            "walk back up: update height, bf = h(left) - h(right)", "if bf > 1: LL -> rotateRight(z); LR -> rotateLeft(left), rotateRight(z)",
                            "if bf < -1: RR -> rotateLeft(z); RL -> rotateRight(right), rotateLeft(z)",
                            "rotateRight(y): x = y.left; y.left = x.right; x.right = y",
                            "rotateLeft(x): y = x.right; x.right = y.left; y.left = x"};
        info_.operationPseudocodeLines["insert"] = {0, 1, 2, 11, 12, 13, 14, 15};
        info_.operationPseudocodeLines["delete"] = {4, 5, 11, 12, 13, 14, 15};
    }

protected:
    void afterInsert(TNode* n) override {
        n->h = 1;
        ev(OpType::Highlight, {Delta::label(n->id, bfText(0))}, {rt(n, Role::Active)}, "New leaf: height 1, balance factor 0.", 11);
        retrace(n->p, true);
    }
    void afterDelete(TNode* parent) override { retrace(parent, false); }

private:
    static int bfOf(const TNode* n) { return hOf(n->l) - hOf(n->r); }
    static string bfText(int bf) { return string("bf=") + (bf > 0 ? "+" : "") + to_string(bf); }

    void updateHeight(TNode* z) { z->h = 1 + max(hOf(z->l), hOf(z->r)); }

    void retrace(TNode* z, bool stopAfterRotation) {
        while (z) {
            updateHeight(z);
            int bf = bfOf(z);
            ev(OpType::Highlight, {Delta::label(z->id, bfText(bf)), ptrD("cur", z)}, {rt(z, abs(bf) > 1 ? Role::Error : Role::Active)},
               "Update node " + k(z) + ": height = " + to_string(z->h) + ", balance factor = " + to_string(hOf(z->l)) + " - " + to_string(hOf(z->r)) + " = " + to_string(bf) +
               (abs(bf) > 1 ? ". |bf| > 1: the tree is unbalanced here!" : "."), 11);
            if (abs(bf) > 1) {
                z = rebalance(z, bf);
                if (stopAfterRotation) { ev(OpType::PointerMove, {ptrD("cur", nullptr)}, {}, "After one rebalance on insert the whole tree is balanced again.", 1); return; }
            }
            z = z->p;
        }
        ev(OpType::PointerMove, {ptrD("cur", nullptr)}, {}, "Reached the root: every balance factor is now in [-1, 1].", 11);
    }

    TNode* rebalance(TNode* z, int bf) {
        if (bf > 1) {
            TNode* y = z->l;
            if (hOf(y->l) >= hOf(y->r)) {
                ev(OpType::Highlight, {}, {rt(z, Role::Error), rt(y, Role::Path)}, "Left-Left (LL) case: the left child is left-heavy. One RIGHT rotation at " + k(z) + " fixes it.", 12);
                return rotateRight(z);
            }
            ev(OpType::Highlight, {}, {rt(z, Role::Error), rt(y, Role::Path)}, "Left-Right (LR) case: the left child is right-heavy. Rotate LEFT at " + k(y) + ", then RIGHT at " + k(z) + ".", 12);
            rotateLeft(y);
            return rotateRight(z);
        }
        TNode* y = z->r;
        if (hOf(y->r) >= hOf(y->l)) {
            ev(OpType::Highlight, {}, {rt(z, Role::Error), rt(y, Role::Path)}, "Right-Right (RR) case: the right child is right-heavy. One LEFT rotation at " + k(z) + " fixes it.", 13);
            return rotateLeft(z);
        }
        ev(OpType::Highlight, {}, {rt(z, Role::Error), rt(y, Role::Path)}, "Right-Left (RL) case: the right child is left-heavy. Rotate RIGHT at " + k(y) + ", then LEFT at " + k(z) + ".", 13);
        rotateRight(y);
        return rotateLeft(z);
    }

    void relabel(TNode* a, TNode* b, const string& note) {
        updateHeight(a); updateHeight(b);
        ev(OpType::Highlight, {Delta::label(a->id, bfText(bfOf(a))), Delta::label(b->id, bfText(bfOf(b)))}, {rt(a, Role::Active), rt(b, Role::Active)}, note, 11);
    }

    TNode* rotateRight(TNode* y) {
        TNode* x = y->l; TNode* t2 = x->r; TNode* p = y->p;
        bool yWasLeft = p && p->l == y;
        vector<Delta> d;
        d.push_back(Delta::edgeAdd(x->id, y->id, Slot::Right));
        if (t2) d.push_back(Delta::edgeAdd(y->id, t2->id, Slot::Left)); else d.push_back(Delta::edgeRemove(y->id, x->id, Slot::Left));
        if (p) d.push_back(Delta::edgeAdd(p->id, x->id, yWasLeft ? Slot::Left : Slot::Right)); else d.push_back(ptrD("root", x));
        x->r = y; y->p = x; y->l = t2; if (t2) t2->p = y; x->p = p;
        if (p) (yWasLeft ? p->l : p->r) = x; else root_ = x;
        ev(OpType::Rotate, d, {rt(x, Role::Moved), rt(y, Role::Moved)}, "RIGHT rotation: " + k(x) + " moves up, " + k(y) + " becomes its right child" + (t2 ? "; " + k(t2) + " moves to be the left child of " + k(y) + "." : "."), 14);
        relabel(y, x, "Recompute heights and balance factors of " + k(y) + " and " + k(x) + ".");
        return x;
    }

    TNode* rotateLeft(TNode* x) {
        TNode* y = x->r; TNode* t2 = y->l; TNode* p = x->p;
        bool xWasLeft = p && p->l == x;
        vector<Delta> d;
        d.push_back(Delta::edgeAdd(y->id, x->id, Slot::Left));
        if (t2) d.push_back(Delta::edgeAdd(x->id, t2->id, Slot::Right)); else d.push_back(Delta::edgeRemove(x->id, y->id, Slot::Right));
        if (p) d.push_back(Delta::edgeAdd(p->id, y->id, xWasLeft ? Slot::Left : Slot::Right)); else d.push_back(ptrD("root", y));
        y->l = x; x->p = y; x->r = t2; if (t2) t2->p = x; y->p = p;
        if (p) (xWasLeft ? p->l : p->r) = y; else root_ = y;
        ev(OpType::Rotate, d, {rt(y, Role::Moved), rt(x, Role::Moved)}, "LEFT rotation: " + k(y) + " moves up, " + k(x) + " becomes its left child" + (t2 ? "; " + k(t2) + " moves to be the right child of " + k(x) + "." : "."), 15);
        relabel(x, y, "Recompute heights and balance factors of " + k(x) + " and " + k(y) + ".");
        return y;
    }
};

inline void registerAvl(AlgorithmCatalog& cat) {
    CatalogEntry e;
    AvlAlgorithm probe;
    e.id = probe.info().id; e.category = "Trees"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Tree; e.ops = TreeBase::ops();
    e.make = [] { return unique_ptr<Algorithm>(new AvlAlgorithm()); };
    cat.add(e);
}

#endif
