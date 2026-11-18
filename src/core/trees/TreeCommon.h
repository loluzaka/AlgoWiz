#ifndef TREECOMMON_H
#define TREECOMMON_H

// Storage and behaviour shared by Binary Tree, BST and AVL: real heap nodes with
// left / right / parent pointers, step helpers, and the three depth-first traversals
// (with the recursion stack highlighted as a path).

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

struct TNode {
    int id = 0;
    int key = 0;
    TNode* l = nullptr;
    TNode* r = nullptr;
    TNode* p = nullptr;
    int h = 1;            // height (AVL)
};

class TreeBase : public StructureAlgorithm {
public:
    const AlgorithmInfo& info() const override { return info_; }

    static vector<OpSpec> ops() {
        return {{"insert", "Insert", {"value"}}, {"search", "Search", {"value"}}, {"delete", "Delete", {"value"}},
                {"min", "Minimum", {}}, {"max", "Maximum", {}},
                {"inorder", "In-order traversal", {}}, {"preorder", "Pre-order traversal", {}}, {"postorder", "Post-order traversal", {}}};
    }

    // ---- read access for the tests ----
    vector<int> inorderKeys() const { vector<int> v; collect(root_, 0, v); return v; }
    vector<int> preorderKeys() const { vector<int> v; collect(root_, 1, v); return v; }
    int height() const { return realHeight(root_); }
    int size() const { return size_; }
    bool isAvlBalanced() const { return balanced(root_); }
    bool isBst() const { vector<int> v = inorderKeys(); return is_sorted(v.begin(), v.end()) && adjacent_find(v.begin(), v.end()) == v.end(); }

protected:
    StructureKind structureKind() const override { return StructureKind::Tree; }
    vector<string> containerNames() const override { return {info_.name}; }

    void begin(const Input&, Recorder& rec) override {
        rec_ = &rec; pool_.clear(); root_ = nullptr; nextId_ = 0; size_ = 0; marked_.clear(); path_.clear();
    }
    void addInitial(int v, Recorder&) override { insertKey(v); finish(""); }
    void cleanup(Recorder& rec) override {
        rec_ = &rec;
        if (marked_.empty()) return;
        vector<Delta> d;
        for (int id : marked_) d.push_back(Delta::mark(id, Role::Default));
        marked_.clear();
        rec.emit(OpType::Unmark, d, {}, "Clear the highlights left by the previous operation.");
    }
    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "insert") { insertKey(c.a); finish("Insert " + to_string(c.a) + " finished."); }
        else if (c.op == "search") { searchKey(c.a); }
        else if (c.op == "delete") { deleteKey(c.a); }
        else if (c.op == "min") extreme(false);
        else if (c.op == "max") extreme(true);
        else if (c.op == "inorder") traverse(0);
        else if (c.op == "preorder") traverse(1);
        else if (c.op == "postorder") traverse(2);
        else throw InvalidOperation("Unknown tree operation: " + c.op);
    }

    virtual void insertKey(int key) = 0;
    virtual void searchKey(int key) = 0;
    virtual void deleteKey(int key) = 0;
    virtual void extreme(bool wantMax) = 0;
    virtual int traversalCodeLine(int order) const { return 8 + order; }

    // ----------------------------------------------------------- helpers
    void ev(OpType op, vector<Delta> d, vector<RoleTag> r, const string& note, int line = -1) { rec_->emit(op, std::move(d), std::move(r), note, line); }
    static RoleTag rt(const TNode* n, Role r) { RoleTag t; t.target_id = n->id; t.role = r; return t; }
    static Delta ptrD(const char* label, const TNode* n) { return Delta::pointer(label, n ? n->id : -1); }
    static string k(const TNode* n) { return to_string(n->key); }
    void requireNonEmpty(const char* what) { if (!root_) throw InvalidOperation(string("Cannot ") + what + ": the tree is empty."); }

    vector<RoleTag> pathRoles(const TNode* cur, Role curRole) const {
        vector<RoleTag> r;
        for (int id : path_) { RoleTag t; t.target_id = id; t.role = Role::Path; r.push_back(t); }
        if (cur) r.push_back(rt(cur, curRole));
        return r;
    }
    void finish(const string& note) {
        path_.clear();
        if (note.empty()) { rec_->emit(OpType::Note, {ptrD("cur", nullptr)}, {}, ""); return; }
        ev(OpType::PointerMove, {ptrD("cur", nullptr)}, {}, note);
    }
    TNode* makeNode(int key) {
        pool_.emplace_back(new TNode());
        TNode* n = pool_.back().get();
        n->id = nextId_++; n->key = key; size_++;
        ev(OpType::NodeCreate, {Delta::create(n->id, key), ptrD("cur", n)}, {rt(n, Role::Active)}, "Create a new node holding " + to_string(key) + ".", 0);
        return n;
    }
    void freeNode(TNode* n) {
        pool_.erase(remove_if(pool_.begin(), pool_.end(), [&](const unique_ptr<TNode>& q) { return q.get() == n; }), pool_.end());
        size_--;
    }
    void setRoot(TNode* n, const string& note) {
        root_ = n;
        if (n) n->p = nullptr;
        ev(OpType::PointerMove, {ptrD("root", n)}, n ? vector<RoleTag>{rt(n, Role::Current)} : vector<RoleTag>{}, note, 0);
    }
    void link(TNode* parent, TNode* child, bool left, const string& note, int line = 0) {
        if (left) parent->l = child; else parent->r = child;
        child->p = parent;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(parent->id, child->id, left ? Slot::Left : Slot::Right)},
           {rt(parent, Role::Path), rt(child, Role::Active)}, note, line);
    }
    void visit(const TNode* n, const string& note, int line) {
        marked_.insert(n->id);
        vector<RoleTag> roles = pathRoles(n, Role::Current);
        ev(OpType::VisitNode, {Delta::mark(n->id, Role::Visited), ptrD("cur", n)}, roles, note, line);
    }

    // ---------------------------------------------------------- traversals
    void traverse(int order) {
        requireNonEmpty("traverse");
        path_.clear();
        trace_.clear();
        walkDfs(root_, order);
        path_.clear();
        const char* names[] = {"In-order (Left, Root, Right)", "Pre-order (Root, Left, Right)", "Post-order (Left, Right, Root)"};
        ev(OpType::PointerMove, {ptrD("cur", nullptr)}, {}, string(names[order]) + " result: " + trace_, traversalCodeLine(order));
    }
    void walkDfs(TNode* n, int order) {
        if (!n) return;
        path_.push_back(n->id);
        auto emitVisit = [&]() {
            trace_ += (trace_.empty() ? "" : ", ") + to_string(n->key);
            path_.pop_back();                            // the node itself is shown as Current, not as Path
            visit(n, "Visit " + to_string(n->key) + ". Output so far: " + trace_, traversalCodeLine(order));
            path_.push_back(n->id);
        };
        if (order == 1) emitVisit();
        if (n->l) { ev(OpType::PointerMove, {ptrD("cur", n->l)}, pathRoles(n->l, Role::Current), "Go to the left child of " + to_string(n->key) + ".", traversalCodeLine(order)); walkDfs(n->l, order); }
        if (order == 0) emitVisit();
        if (n->r) { ev(OpType::PointerMove, {ptrD("cur", n->r)}, pathRoles(n->r, Role::Current), "Go to the right child of " + to_string(n->key) + ".", traversalCodeLine(order)); walkDfs(n->r, order); }
        if (order == 2) emitVisit();
        path_.pop_back();
    }

    // ---------------------------------------------------------- model queries
    static int hOf(const TNode* n) { return n ? n->h : 0; }
    static void collect(const TNode* n, int order, vector<int>& out) {
        if (!n) return;
        if (order == 1) out.push_back(n->key);
        collect(n->l, order, out);
        if (order == 0) out.push_back(n->key);
        collect(n->r, order, out);
        if (order == 2) out.push_back(n->key);
    }
    static int realHeight(const TNode* n) { return n ? 1 + max(realHeight(n->l), realHeight(n->r)) : 0; }
    static bool balanced(const TNode* n) {
        if (!n) return true;
        int d = realHeight(n->l) - realHeight(n->r);
        return d >= -1 && d <= 1 && balanced(n->l) && balanced(n->r);
    }

    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    vector<unique_ptr<TNode>> pool_;
    TNode* root_ = nullptr;
    int nextId_ = 0, size_ = 0;
    set<int> marked_;
    vector<int> path_;
    string trace_;
};

#endif
