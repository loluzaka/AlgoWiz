#ifndef STACK_H
#define STACK_H

// Array-based stack with a fixed capacity. Overflow and underflow are shown as steps
// (red cells + explanation) instead of being silently ignored.

#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

class StackAlgorithm : public StructureAlgorithm {
public:
    StackAlgorithm() {
        info_.id = "stack"; info_.name = "Stack (array)"; info_.category = "Stack";
        info_.best = "O(1)"; info_.average = "O(1)"; info_.worst = "O(1)"; info_.space = "O(n)";
        info_.description = "Last In, First Out. push and pop only touch the TOP element, so both run in constant time.";
        info_.pseudocode = {"push(x): if top == cap-1: OVERFLOW", "  top++; a[top] = x", "pop(): if top == -1: UNDERFLOW", "  x = a[top]; top--; return x",
                            "peek(): if top == -1: UNDERFLOW; return a[top]", "isEmpty(): return top == -1", "isFull(): return top == cap-1"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    string validate(const Input& in) const override {
        if (in.capacity < 1 || in.capacity > 12) return "Stack capacity must be between 1 and 12.";
        if ((int)in.values.size() > in.capacity) return "More initial values than the stack capacity (" + to_string(in.capacity) + ").";
        return "";
    }
    static vector<OpSpec> ops() {
        return {{"push", "Push", {"value"}}, {"pop", "Pop", {}}, {"peek", "Peek", {}}, {"is_empty", "Is Empty?", {}}, {"is_full", "Is Full?", {}}};
    }
    const vector<int>& contents() const { return a_; }   // bottom .. top

protected:
    StructureKind structureKind() const override { return StructureKind::Stack; }
    vector<string> containerNames() const override { return {"Stack"}; }

    void begin(const Input& in, Recorder& rec) override {
        rec_ = &rec; cap_ = in.capacity; a_.clear();
        rec.setup({Delta::resize(cap_, 0)});
    }
    void addInitial(int v, Recorder&) override {
        a_.push_back(v);
        rec_->setup({Delta::value((int)a_.size() - 1, v, 0), Delta::pointer("TOP", (int)a_.size() - 1, 0)});
    }

    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        int top = (int)a_.size() - 1;
        if (c.op == "push") {
            rec.emit(OpType::Compare, {}, top >= 0 ? vector<RoleTag>{tag(top, Role::Compared)} : vector<RoleTag>{},
                     "Is the stack full? TOP = " + to_string(top) + ", capacity - 1 = " + to_string(cap_ - 1) + ".", 0);
            if (top == cap_ - 1) {
                rec.emit(OpType::Note, {}, {tag(top, Role::Error)}, "OVERFLOW: the stack is full, " + to_string(c.a) + " cannot be pushed.", 0);
                return;
            }
            rec.emit(OpType::PointerMove, {Delta::pointer("TOP", top + 1, 0)}, {tag(top + 1, Role::Current)}, "TOP moves up to index " + to_string(top + 1) + ".", 1);
            a_.push_back(c.a);
            rec.emit(OpType::Insert, {Delta::value(top + 1, c.a, 0)}, {tag(top + 1, Role::Moved)}, "Store " + to_string(c.a) + " at a[" + to_string(top + 1) + "].", 1);
        } else if (c.op == "pop") {
            rec.emit(OpType::Compare, {}, top >= 0 ? vector<RoleTag>{tag(top, Role::Compared)} : vector<RoleTag>{},
                     "Is the stack empty? TOP = " + to_string(top) + ".", 2);
            if (top < 0) { rec.emit(OpType::Note, {}, {}, "UNDERFLOW: the stack is empty, there is nothing to pop.", 2); return; }
            int x = a_.back();
            rec.emit(OpType::Highlight, {}, {tag(top, Role::Candidate)}, "Pop " + to_string(x) + " from the top.", 3);
            a_.pop_back();
            rec.emit(OpType::Delete, {Delta::destroy(top, 0), Delta::pointer("TOP", top - 1, 0)}, {}, "Remove a[" + to_string(top) + "]; TOP moves down to " + to_string(top - 1) + ". Returned " + to_string(x) + ".", 3);
        } else if (c.op == "peek") {
            if (top < 0) { rec.emit(OpType::Note, {}, {}, "UNDERFLOW: the stack is empty, there is nothing to peek.", 4); return; }
            rec.emit(OpType::Access, {}, {tag(top, Role::Found)}, "Peek: the top element is " + to_string(a_.back()) + " (not removed).", 4);
        } else if (c.op == "is_empty") {
            rec.emit(OpType::Compare, {}, {}, string("isEmpty() = ") + (top < 0 ? "true" : "false") + " (TOP = " + to_string(top) + ").", 5);
        } else if (c.op == "is_full") {
            rec.emit(OpType::Compare, {}, {}, string("isFull() = ") + (top == cap_ - 1 ? "true" : "false") + " (TOP = " + to_string(top) + ", capacity " + to_string(cap_) + ").", 6);
        } else {
            throw InvalidOperation("Unknown stack operation: " + c.op);
        }
    }

private:
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    vector<int> a_;
    int cap_ = 8;
};

inline void registerStack(AlgorithmCatalog& cat) {
    CatalogEntry e;
    StackAlgorithm probe;
    e.id = probe.info().id; e.category = "Stack"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Stack; e.ops = StackAlgorithm::ops();
    e.make = [] { return unique_ptr<Algorithm>(new StackAlgorithm()); };
    cat.add(e);
}

#endif
