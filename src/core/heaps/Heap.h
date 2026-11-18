#ifndef HEAP_H
#define HEAP_H

// Binary Min-Heap, Max-Heap and Priority Queue (a min-heap where the smallest number has
// the highest priority). The heap lives in an array; the renderer draws the same data
// as a tree using the index rule  parent(i) = (i-1)/2,  children 2i+1 and 2i+2.

#include <sstream>
#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

enum class HeapKind { Min, Max, PriorityQueue };

class HeapAlgorithm : public StructureAlgorithm {
public:
    static const int MAX_SIZE = 15;

    explicit HeapAlgorithm(HeapKind k) : kind_(k) {
        info_.category = "Heaps";
        info_.best = "O(1) peek"; info_.average = "O(log n)"; info_.worst = "O(log n)"; info_.space = "O(n)";
        switch (k) {
        case HeapKind::Min: info_.id = "min_heap"; info_.name = "Min Heap";
            info_.description = "A complete binary tree stored in an array where every parent is <= its children, so the minimum is always at the root."; break;
        case HeapKind::Max: info_.id = "max_heap"; info_.name = "Max Heap";
            info_.description = "A complete binary tree stored in an array where every parent is >= its children, so the maximum is always at the root."; break;
        case HeapKind::PriorityQueue: info_.id = "priority_queue"; info_.name = "Priority Queue (min-heap)";
            info_.description = "A queue where the item with the SMALLEST number (highest priority) leaves first. Implemented with a binary min-heap."; break;
        }
        info_.pseudocode = {"insert(x): a[n] = x; n++; siftUp(n-1)", "siftUp(i): while i > 0 and a[i] beats a[parent]: swap; i = parent",
                            "extract(): root = a[0]; a[0] = a[n-1]; n--; siftDown(0); return root", "siftDown(i): pick the best of a[i], a[2i+1], a[2i+2]; if not a[i]: swap and continue",
                            "peek(): return a[0]", "buildHeap(): for i = n/2-1 down to 0: siftDown(i)"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    vector<OpSpec> opSpecs() const {
        if (kind_ == HeapKind::PriorityQueue)
            return {{"insert", "Enqueue (insert with priority)", {"priority value"}}, {"extract", "Dequeue (highest priority)", {}}, {"peek", "Peek highest priority", {}}};
        OpSpec build{"build_heap", "Build heap from a list", {}, true};
        return {{"insert", "Insert", {"value"}}, {"extract", kind_ == HeapKind::Min ? "Extract minimum" : "Extract maximum", {}}, {"peek", "Peek", {}},
                build, {"heapify", "Heapify (sift down) at index", {"index"}}};
    }
    const vector<int>& contents() const { return a_; }
    bool isHeap() const {
        for (int i = 1; i < (int)a_.size(); i++) if (better(a_[i], a_[(i - 1) / 2])) return false;
        return true;
    }

protected:
    StructureKind structureKind() const override { return StructureKind::Heap; }
    vector<string> containerNames() const override { return {"Heap array"}; }

    void begin(const Input&, Recorder& rec) override { rec_ = &rec; a_.clear(); rec.setup({Delta::resize(0, 0)}); }
    void addInitial(int v, Recorder&) override { if ((int)a_.size() < MAX_SIZE) insertValue(v); }
    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "insert") insertValue(c.a);
        else if (c.op == "extract") extractRoot();
        else if (c.op == "peek") peek();
        else if (c.op == "build_heap") buildHeap(c.s);
        else if (c.op == "heapify") {
            if (a_.empty()) throw InvalidOperation("Cannot heapify: the heap is empty.");
            if (c.a < 0 || c.a >= (int)a_.size()) throw InvalidOperation("Invalid index. Please enter an index between 0 and " + to_string(a_.size() - 1) + ".");
            ev(OpType::Heapify, {}, {tag(c.a, Role::Active)}, "Heapify (sift down) from index " + to_string(c.a) + ".", 3);
            siftDown(c.a, (int)a_.size());
            ev(OpType::PointerMove, {Delta::pointer("i", -1, 0)}, {}, "Heapify finished.", 3);
        } else throw InvalidOperation("Unknown heap operation: " + c.op);
    }

private:
    bool better(int x, int y) const { return kind_ == HeapKind::Max ? x > y : x < y; }
    string word() const { return kind_ == HeapKind::Max ? "larger" : "smaller"; }
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    void ev(OpType op, vector<Delta> d, vector<RoleTag> r, const string& note, int line = -1) { rec_->emit(op, std::move(d), std::move(r), note, line); }
    void swapSteps(int i, int j, const string& note, int line) {
        std::swap(a_[i], a_[j]);
        ev(OpType::Swap, {Delta::value(i, a_[i], 0), Delta::value(j, a_[j], 0)}, {tag(i, Role::Moved), tag(j, Role::Moved)}, note, line);
    }

    void siftUp(int i) {
        ev(OpType::Heapify, {Delta::pointer("i", i, 0)}, {tag(i, Role::Current)}, "Sift up from index " + to_string(i) + ": the new value may be " + word() + " than its parent.", 1);
        while (i > 0) {
            int p = (i - 1) / 2;
            bool swapIt = better(a_[i], a_[p]);
            ev(OpType::Compare, {}, {tag(i, Role::Compared), tag(p, Role::Compared)},
               "Compare " + to_string(a_[i]) + " with its parent " + to_string(a_[p]) + (swapIt ? ": it is " + word() + ", so swap." : ": heap property holds, stop."), 1);
            if (!swapIt) break;
            swapSteps(i, p, "Swap " + to_string(a_[i]) + " and " + to_string(a_[p]) + ".", 1);
            i = p;
            ev(OpType::PointerMove, {Delta::pointer("i", i, 0)}, {tag(i, Role::Current)}, "Continue from index " + to_string(i) + ".", 1);
        }
        ev(OpType::PointerMove, {Delta::pointer("i", -1, 0)}, {tag(i, Role::Sorted)}, "The value settled at index " + to_string(i) + ".", 1);
    }

    void siftDown(int i, int n) {
        ev(OpType::PointerMove, {Delta::pointer("i", i, 0)}, {tag(i, Role::Current)}, "Sift down from index " + to_string(i) + ".", 3);
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, best = i;
            if (l < n) {
                bool b = better(a_[l], a_[best]);
                ev(OpType::Compare, {}, {tag(l, Role::Compared), tag(best, Role::Compared)}, "Left child " + to_string(a_[l]) + " vs " + to_string(a_[best]) + (b ? ": child is " + word() + "." : ": parent keeps its place."), 3);
                if (b) best = l;
            }
            if (r < n) {
                bool b = better(a_[r], a_[best]);
                ev(OpType::Compare, {}, {tag(r, Role::Compared), tag(best, Role::Compared)}, "Right child " + to_string(a_[r]) + " vs " + to_string(a_[best]) + (b ? ": right child is " + word() + "." : ": no change."), 3);
                if (b) best = r;
            }
            if (best == i) { ev(OpType::PointerMove, {Delta::pointer("i", -1, 0)}, {tag(i, Role::Sorted)}, "Index " + to_string(i) + " is in place; the heap property holds below it.", 3); return; }
            swapSteps(i, best, "Swap " + to_string(a_[i]) + " and " + to_string(a_[best]) + ".", 3);
            i = best;
            ev(OpType::PointerMove, {Delta::pointer("i", i, 0)}, {tag(i, Role::Current)}, "Continue from index " + to_string(i) + ".", 3);
        }
    }

    void insertValue(int v) {
        if ((int)a_.size() >= MAX_SIZE) throw InvalidOperation("Overflow: the heap is full (maximum " + to_string(MAX_SIZE) + " elements).");
        int n = (int)a_.size();
        a_.push_back(v);
        ev(OpType::Insert, {Delta::resize(n + 1, 0), Delta::value(n, v, 0)}, {tag(n, Role::Active)}, "Place " + to_string(v) + " at the end of the array (index " + to_string(n) + ").", 0);
        siftUp(n);
    }

    void extractRoot() {
        if (a_.empty()) throw InvalidOperation("Underflow: the heap is empty.");
        int n = (int)a_.size(), root = a_[0];
        ev(OpType::Highlight, {}, {tag(0, Role::Candidate)}, "Extract the root " + to_string(root) + ".", 2);
        if (n == 1) { a_.clear(); ev(OpType::Delete, {Delta::resize(0, 0)}, {}, "The heap is now empty. Returned " + to_string(root) + ".", 2); return; }
        a_[0] = a_[n - 1];
        a_.pop_back();
        ev(OpType::Delete, {Delta::value(0, a_[0], 0), Delta::resize(n - 1, 0)}, {tag(0, Role::Moved)},
           "Move the last element " + to_string(a_[0]) + " to the root and shrink the heap to " + to_string(n - 1) + ". Returned " + to_string(root) + ".", 2);
        siftDown(0, n - 1);
    }

    void peek() {
        if (a_.empty()) throw InvalidOperation("Underflow: the heap is empty.");
        ev(OpType::Access, {}, {tag(0, Role::Found)}, "Peek: the root is " + to_string(a_[0]) + " (not removed).", 4);
    }

    void buildHeap(const string& list) {
        vector<int> vals;
        istringstream is(list);
        int x;
        while (is >> x) vals.push_back(x);
        if (vals.empty()) throw InvalidOperation("Please enter at least one number to build a heap.");
        if ((int)vals.size() > MAX_SIZE) throw InvalidOperation("At most " + to_string(MAX_SIZE) + " numbers fit in the heap view.");
        a_ = vals;
        vector<Delta> d = {Delta::clear(0), Delta::resize((int)vals.size(), 0)};
        for (size_t i = 0; i < vals.size(); i++) d.push_back(Delta::value((int)i, vals[i], 0));
        ev(OpType::Assign, d, {}, "Load the numbers into the array as they are (not yet a heap).", 5);
        int n = (int)a_.size();
        for (int i = n / 2 - 1; i >= 0; i--) {
            ev(OpType::Heapify, {}, {tag(i, Role::Active)}, "Build heap: heapify the subtree rooted at index " + to_string(i) + " (the last non-leaf is index " + to_string(n / 2 - 1) + ").", 5);
            siftDown(i, n);
        }
        ev(OpType::PointerMove, {Delta::pointer("i", -1, 0)}, {}, "Heap built in O(n).", 5);
    }

    HeapKind kind_;
    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    vector<int> a_;
};

inline void registerHeaps(AlgorithmCatalog& cat) {
    for (HeapKind k : {HeapKind::Min, HeapKind::Max, HeapKind::PriorityQueue}) {
        CatalogEntry e;
        HeapAlgorithm probe(k);
        e.id = probe.info().id; e.category = "Heaps"; e.name = probe.info().name;
        e.input = InputKind::Structure; e.structure = StructureKind::Heap; e.ops = probe.opSpecs();
        e.make = [k] { return unique_ptr<Algorithm>(new HeapAlgorithm(k)); };
        cat.add(e);
    }
}

#endif
