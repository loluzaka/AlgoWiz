#ifndef ARRAYSTRUCTURE_H
#define ARRAYSTRUCTURE_H

// The "Arrays" entry of the home menu: a dynamic array whose insert / delete shift elements
// one cell at a time so the learner sees why those operations cost O(n).

#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

class ArrayStructure : public StructureAlgorithm {
public:
    static const int MAX_SIZE = 20;

    ArrayStructure() {
        info_.id = "array"; info_.name = "Array"; info_.category = "Arrays";
        info_.best = "O(1) access"; info_.average = "O(n) insert/delete"; info_.worst = "O(n)"; info_.space = "O(n)";
        info_.description = "Contiguous indexed cells. Access by index is instant; inserting or deleting in the middle shifts the cells after it.";
        info_.pseudocode = {"access: return a[i]", "update: a[i] = value", "insert at p: for i = n down to p+1: a[i] = a[i-1]; a[p] = value; n++",
                            "delete at p: for i = p to n-2: a[i] = a[i+1]; n--", "traverse: for i = 0 .. n-1: visit a[i]"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    static vector<OpSpec> ops() {
        return {{"insert", "Insert at position", {"position (0 = first)", "value"}}, {"delete", "Delete at position", {"position (0 = first)"}},
                {"update", "Update value at position", {"position", "new value"}}, {"access", "Access (read) position", {"position"}},
                {"traverse", "Traverse all cells", {}}};
    }
    const vector<int>& contents() const { return a_; }

protected:
    StructureKind structureKind() const override { return StructureKind::Array; }
    vector<string> containerNames() const override { return {"Array"}; }

    void begin(const Input&, Recorder& rec) override {
        rec_ = &rec; a_.clear();
        rec.setup({Delta::resize(0, 0)});
    }
    void addInitial(int v, Recorder&) override {
        if ((int)a_.size() >= MAX_SIZE) return;
        a_.push_back(v);
        rec_->setup({Delta::resize((int)a_.size(), 0), Delta::value((int)a_.size() - 1, v, 0)});
    }

    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        int n = (int)a_.size();
        if (c.op == "insert") {
            if (n >= MAX_SIZE) throw InvalidOperation("Overflow: the array already holds the maximum of " + to_string(MAX_SIZE) + " elements.");
            if (c.a < 0 || c.a > n) throw InvalidOperation("Invalid position. Please enter a position between 0 and " + to_string(n) + ".");
            a_.push_back(0);
            rec.emit(OpType::Insert, {Delta::resize(n + 1, 0)}, {tag(n, Role::Active)}, "Make room: the array grows from " + to_string(n) + " to " + to_string(n + 1) + " cells.", 2);
            for (int i = n; i > c.a; i--) {
                a_[i] = a_[i - 1];
                rec.emit(OpType::Assign, {Delta::value(i, a_[i], 0)}, {tag(i - 1, Role::Compared), tag(i, Role::Moved)},
                         "Shift " + to_string(a_[i]) + " from index " + to_string(i - 1) + " to index " + to_string(i) + ".", 2);
            }
            a_[c.a] = c.b;
            rec.emit(OpType::Insert, {Delta::value(c.a, c.b, 0)}, {tag(c.a, Role::Found)}, "Write " + to_string(c.b) + " at index " + to_string(c.a) + ".", 2);
        } else if (c.op == "delete") {
            if (n == 0) throw InvalidOperation("Underflow: the array is empty.");
            if (c.a < 0 || c.a >= n) throw InvalidOperation("Invalid position. Please enter a position between 0 and " + to_string(n - 1) + ".");
            rec.emit(OpType::Highlight, {}, {tag(c.a, Role::Candidate)}, "Delete " + to_string(a_[c.a]) + " at index " + to_string(c.a) + ".", 3);
            for (int i = c.a; i < n - 1; i++) {
                a_[i] = a_[i + 1];
                rec.emit(OpType::Assign, {Delta::value(i, a_[i], 0)}, {tag(i + 1, Role::Compared), tag(i, Role::Moved)},
                         "Shift " + to_string(a_[i]) + " from index " + to_string(i + 1) + " to index " + to_string(i) + ".", 3);
            }
            a_.pop_back();
            rec.emit(OpType::Delete, {Delta::resize(n - 1, 0)}, {}, "The array shrinks to " + to_string(n - 1) + " cells.", 3);
        } else if (c.op == "update") {
            if (c.a < 0 || c.a >= n) throw InvalidOperation("Invalid position. " + (n ? "Please enter a position between 0 and " + to_string(n - 1) + "." : string("The array is empty.")));
            a_[c.a] = c.b;
            rec.emit(OpType::Assign, {Delta::value(c.a, c.b, 0)}, {tag(c.a, Role::Moved)}, "a[" + to_string(c.a) + "] = " + to_string(c.b) + ".", 1);
        } else if (c.op == "access") {
            if (c.a < 0 || c.a >= n) throw InvalidOperation("Invalid position. " + (n ? "Please enter a position between 0 and " + to_string(n - 1) + "." : string("The array is empty.")));
            rec.emit(OpType::Access, {}, {tag(c.a, Role::Found)}, "a[" + to_string(c.a) + "] = " + to_string(a_[c.a]) + " (direct access, O(1)).", 0);
        } else if (c.op == "traverse") {
            if (n == 0) throw InvalidOperation("Cannot traverse: the array is empty.");
            string order;
            for (int i = 0; i < n; i++) {
                order += (i ? ", " : "") + to_string(a_[i]);
                rec.emit(OpType::VisitNode, {Delta::pointer("i", i, 0)}, {tag(i, Role::Visited)}, "Visit a[" + to_string(i) + "] = " + to_string(a_[i]) + ". Order so far: " + order, 4);
            }
            rec.emit(OpType::PointerMove, {Delta::pointer("i", -1, 0)}, {}, "Traversal complete: " + order, 4);
        } else {
            throw InvalidOperation("Unknown array operation: " + c.op);
        }
    }

private:
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    AlgorithmInfo info_;
    vector<int> a_;
    Recorder* rec_ = nullptr;
};

inline void registerArrays(AlgorithmCatalog& cat) {
    CatalogEntry e;
    ArrayStructure probe;
    e.id = probe.info().id; e.category = "Arrays"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Array; e.ops = ArrayStructure::ops();
    e.make = [] { return unique_ptr<Algorithm>(new ArrayStructure()); };
    cat.add(e);
}

#endif
