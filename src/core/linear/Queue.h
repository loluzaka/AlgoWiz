#ifndef QUEUE_H
#define QUEUE_H

// Linear (array) queue and circular queue in one class. The linear queue deliberately
// keeps the classic flaw: after dequeues the freed front cells cannot be reused, so
// "overflow" can occur with empty cells in front. The circular queue wraps REAR around.

#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

class QueueAlgorithm : public StructureAlgorithm {
public:
    explicit QueueAlgorithm(bool circular) : circular_(circular) {
        info_.id = circular ? "circular_queue" : "queue";
        info_.name = circular ? "Circular Queue" : "Queue (linear array)";
        info_.category = "Queue";
        info_.best = "O(1)"; info_.average = "O(1)"; info_.worst = "O(1)"; info_.space = "O(n)";
        info_.description = circular
            ? "First In, First Out in a ring: FRONT and REAR wrap around with modulo arithmetic so freed cells are reused."
            : "First In, First Out. Items enter at REAR and leave at FRONT; freed front cells are NOT reused in this linear version.";
        info_.pseudocode = circular
            ? vector<string>{"enqueue(x): if count == cap: OVERFLOW", "  rear = (rear+1) % cap; a[rear] = x; count++", "dequeue(): if count == 0: UNDERFLOW",
                             "  x = a[front]; front = (front+1) % cap; count--", "front(): return a[front]", "rear(): return a[rear]", "isEmpty(): count == 0 / isFull(): count == cap"}
            : vector<string>{"enqueue(x): if rear == cap-1: OVERFLOW", "  rear++; a[rear] = x", "dequeue(): if empty: UNDERFLOW",
                             "  x = a[front]; front++", "front(): return a[front]", "rear(): return a[rear]", "isEmpty(): front > rear / isFull(): rear == cap-1"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    string validate(const Input& in) const override {
        if (in.capacity < 1 || in.capacity > 12) return "Queue capacity must be between 1 and 12.";
        if ((int)in.values.size() > in.capacity) return "More initial values than the queue capacity (" + to_string(in.capacity) + ").";
        return "";
    }
    static vector<OpSpec> ops() {
        return {{"enqueue", "Enqueue", {"value"}}, {"dequeue", "Dequeue", {}}, {"front", "Front", {}}, {"rear", "Rear", {}},
                {"is_empty", "Is Empty?", {}}, {"is_full", "Is Full?", {}}};
    }
    vector<int> contents() const {              // FRONT .. REAR
        vector<int> r;
        for (int i = 0; i < count_; i++) r.push_back(slot_[(front_ + i) % cap_]);
        return r;
    }

protected:
    StructureKind structureKind() const override { return StructureKind::Queue; }
    vector<string> containerNames() const override { return {circular_ ? "Circular queue" : "Queue"}; }

    void begin(const Input& in, Recorder& rec) override {
        rec_ = &rec; cap_ = in.capacity; slot_.assign(cap_, 0); front_ = 0; rear_ = -1; count_ = 0;
        rec.setup({Delta::resize(cap_, 0)});
    }
    void addInitial(int v, Recorder&) override {
        rear_ = circular_ ? (rear_ + 1) % cap_ : rear_ + 1;
        slot_[rear_] = v; count_++;
        rec_->setup({Delta::value(rear_, v, 0), Delta::pointer("FRONT", front_, 0), Delta::pointer("REAR", rear_, 0)});
    }

    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "enqueue") enqueue(c.a);
        else if (c.op == "dequeue") dequeue();
        else if (c.op == "front") peek(true);
        else if (c.op == "rear") peek(false);
        else if (c.op == "is_empty") rec.emit(OpType::Compare, {}, {}, string("isEmpty() = ") + (count_ == 0 ? "true" : "false") + ".", 6);
        else if (c.op == "is_full") rec.emit(OpType::Compare, {}, {}, string("isFull() = ") + (full() ? "true" : "false") + ".", 6);
        else throw InvalidOperation("Unknown queue operation: " + c.op);
    }

private:
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    bool full() const { return circular_ ? count_ == cap_ : rear_ == cap_ - 1; }

    void enqueue(int x) {
        string why = circular_ ? "count = " + to_string(count_) + ", capacity = " + to_string(cap_)
                               : "REAR = " + to_string(rear_) + ", capacity - 1 = " + to_string(cap_ - 1);
        rec_->emit(OpType::Compare, {}, {}, "Is the queue full? " + why + ".", 0);
        if (full()) {
            vector<RoleTag> r;
            if (rear_ >= 0) r.push_back(tag(rear_, Role::Error));
            rec_->emit(OpType::Note, {}, r, string("OVERFLOW: cannot enqueue ") + to_string(x) + "." +
                       (!circular_ && count_ < cap_ ? " (Cells in front are empty but a linear queue cannot reuse them - this is why circular queues exist.)" : ""), 0);
            return;
        }
        int nr = circular_ ? (rear_ + 1) % cap_ : rear_ + 1;
        vector<Delta> d = {Delta::pointer("REAR", nr, 0)};
        if (count_ == 0) { front_ = nr; d.push_back(Delta::pointer("FRONT", nr, 0)); }
        rec_->emit(OpType::PointerMove, d, {tag(nr, Role::Current)}, "REAR moves to index " + to_string(nr) + (circular_ ? " ((REAR + 1) % capacity)." : "."), 1);
        rear_ = nr; slot_[nr] = x; count_++;
        rec_->emit(OpType::Insert, {Delta::value(nr, x, 0)}, {tag(nr, Role::Moved)}, "Store " + to_string(x) + " at index " + to_string(nr) + ".", 1);
    }

    void dequeue() {
        rec_->emit(OpType::Compare, {}, {}, "Is the queue empty? count = " + to_string(count_) + ".", 2);
        if (count_ == 0) { rec_->emit(OpType::Note, {}, {}, "UNDERFLOW: the queue is empty, there is nothing to dequeue.", 2); return; }
        int x = slot_[front_], old = front_;
        rec_->emit(OpType::Highlight, {}, {tag(old, Role::Candidate)}, "Dequeue " + to_string(x) + " from FRONT (index " + to_string(old) + ").", 3);
        count_--;
        if (count_ == 0) {
            rec_->emit(OpType::Delete, {Delta::destroy(old, 0), Delta::pointer("FRONT", -1, 0), Delta::pointer("REAR", -1, 0)}, {},
                       "The queue is now empty; FRONT and REAR are reset. Returned " + to_string(x) + ".", 3);
            front_ = 0; rear_ = -1;
        } else {
            front_ = circular_ ? (front_ + 1) % cap_ : front_ + 1;
            rec_->emit(OpType::Delete, {Delta::destroy(old, 0), Delta::pointer("FRONT", front_, 0)}, {tag(front_, Role::Current)},
                       "FRONT moves to index " + to_string(front_) + ". Returned " + to_string(x) + ".", 3);
        }
    }

    void peek(bool wantFront) {
        if (count_ == 0) { rec_->emit(OpType::Note, {}, {}, "UNDERFLOW: the queue is empty.", wantFront ? 4 : 5); return; }
        int idx = wantFront ? front_ : rear_;
        rec_->emit(OpType::Access, {}, {tag(idx, Role::Found)}, string(wantFront ? "Front" : "Rear") + " element is " + to_string(slot_[idx]) + " (index " + to_string(idx) + ").", wantFront ? 4 : 5);
    }

    bool circular_;
    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    vector<int> slot_;
    int cap_ = 8, front_ = 0, rear_ = -1, count_ = 0;
};

inline void registerQueue(AlgorithmCatalog& cat) {
    for (bool circ : {false, true}) {
        CatalogEntry e;
        QueueAlgorithm probe(circ);
        e.id = probe.info().id; e.category = "Queue"; e.name = probe.info().name;
        e.input = InputKind::Structure; e.structure = StructureKind::Queue; e.ops = QueueAlgorithm::ops();
        e.make = [circ] { return unique_ptr<Algorithm>(new QueueAlgorithm(circ)); };
        cat.add(e);
    }
}

#endif
