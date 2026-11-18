#ifndef LINKEDLIST_H
#define LINKEDLIST_H

// Singly, doubly and circular (singly) linked lists with real heap nodes and real
// pointers. Every operation both performs the pointer surgery on the nodes AND records
// the corresponding Steps (create node, set next, move HEAD ...).

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

class LinkedListAlgorithm : public StructureAlgorithm {
public:
    enum class Type { Singly, Doubly, Circular };

    explicit LinkedListAlgorithm(Type t) : type_(t) {
        info_.category = "Linked Lists";
        info_.best = "O(1) at head"; info_.average = "O(n)"; info_.worst = "O(n)"; info_.space = "O(n)";
        switch (t) {
        case Type::Singly:   info_.id = "singly_list";   info_.name = "Singly Linked List";
            info_.description = "Each node stores a value and a NEXT pointer. HEAD points to the first node, the last node points to NULL."; break;
        case Type::Doubly:   info_.id = "doubly_list";   info_.name = "Doubly Linked List";
            info_.description = "Each node has NEXT and PREV pointers, so the list can be walked in both directions and TAIL can be reached directly."; break;
        case Type::Circular: info_.id = "circular_list"; info_.name = "Circular Linked List";
            info_.description = "A singly linked list whose last node points back to HEAD instead of NULL."; break;
        }
        info_.pseudocode = {"insert at beginning: new.next = head; head = new", "insert at end: walk to last; last.next = new", "insert at position: walk to pos-1; new.next = prev.next; prev.next = new",
                            "delete first: head = head.next", "delete last: walk to second last; prev.next = NULL", "delete at position: prev.next = prev.next.next",
                            "search: cur = head; while cur: compare cur.value", "traverse: visit every node from head"};
    }
    const AlgorithmInfo& info() const override { return info_; }

    static vector<OpSpec> ops() {
        return {{"insert_begin", "Insert at beginning", {"value"}}, {"insert_end", "Insert at end", {"value"}},
                {"insert_pos", "Insert at position", {"position (0 = first)", "value"}},
                {"delete_begin", "Delete from beginning", {}}, {"delete_end", "Delete from end", {}},
                {"delete_pos", "Delete from position", {"position (0 = first)"}},
                {"search", "Search for a value", {"value"}}, {"traverse", "Traverse", {}}};
    }
    vector<int> values() const {          // contents in list order (used by the tests)
        vector<int> r;
        Node* c = head_;
        for (int i = 0; i < size_ && c; i++, c = c->next) r.push_back(c->value);
        return r;
    }

protected:
    StructureKind structureKind() const override { return StructureKind::List; }
    vector<string> containerNames() const override { return {info_.name}; }

    void begin(const Input&, Recorder& rec) override {
        rec_ = &rec; pool_.clear(); head_ = tail_ = nullptr; size_ = 0; nextId_ = 0;
    }
    void addInitial(int v, Recorder&) override { insertEnd(v); finish("", {}); }

    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "insert_begin") { insertBegin(c.a); finish("Inserted " + to_string(c.a) + " at the beginning.", {}); }
        else if (c.op == "insert_end") { insertEnd(c.a); finish("Inserted " + to_string(c.a) + " at the end.", {}); }
        else if (c.op == "insert_pos") { insertPos(c.a, c.b); finish("Inserted " + to_string(c.b) + " at position " + to_string(c.a) + ".", {}); }
        else if (c.op == "delete_begin") { deleteBegin(); finish("Deleted the first node.", {}); }
        else if (c.op == "delete_end") { deleteEnd(); finish("Deleted the last node.", {}); }
        else if (c.op == "delete_pos") { deletePos(c.a); finish("Deleted the node at position " + to_string(c.a) + ".", {}); }
        else if (c.op == "search") search(c.a);
        else if (c.op == "traverse") traverse();
        else throw InvalidOperation("Unknown list operation: " + c.op);
    }

private:
    struct Node { int id; int value; Node* next = nullptr; Node* prev = nullptr; };

    // ------------------------------------------------------------ helpers
    void ev(OpType op, vector<Delta> d, vector<RoleTag> r, const string& note, int line = -1) { rec_->emit(op, std::move(d), std::move(r), note, line); }
    RoleTag rt(Node* n, Role r) { RoleTag t; t.target_id = n->id; t.role = r; return t; }
    static Delta ptrD(const char* label, Node* n) { return Delta::pointer(label, n ? n->id : -1); }
    string v(Node* n) { return to_string(n->value); }

    Node* makeNode(int value) {
        pool_.emplace_back(new Node());
        Node* n = pool_.back().get();
        n->id = nextId_++; n->value = value;
        ev(OpType::NodeCreate, {Delta::create(n->id, value), ptrD("new", n)}, {rt(n, Role::Active)}, "Create a new node holding " + to_string(value) + ".");
        return n;
    }
    void freeNode(Node* n) {
        pool_.erase(remove_if(pool_.begin(), pool_.end(), [&](const unique_ptr<Node>& p) { return p.get() == n; }), pool_.end());
    }
    Node* walk(int hops) {                        // move `cur` from HEAD forward `hops` nodes
        Node* c = head_;
        ev(OpType::VisitNode, {ptrD("cur", c)}, {rt(c, Role::Current)}, "cur starts at HEAD (value " + v(c) + ").", 6);
        for (int i = 0; i < hops; i++) {
            c = c->next;
            ev(OpType::VisitNode, {ptrD("cur", c)}, {rt(c, Role::Current)}, "cur moves to the next node (value " + v(c) + ").", 6);
        }
        return c;
    }
    void finish(const string& note, vector<RoleTag> roles) {
        if (note.empty()) { rec_->emit(OpType::Note, {ptrD("cur", nullptr), ptrD("new", nullptr), ptrD("del", nullptr)}, {}, ""); return; }
        ev(OpType::PointerMove, {ptrD("cur", nullptr), ptrD("new", nullptr), ptrD("del", nullptr)}, std::move(roles), note);
    }
    void linkFirstNode(Node* n) {
        head_ = tail_ = n;
        vector<Delta> d = {ptrD("HEAD", n), ptrD("TAIL", n)};
        if (type_ == Type::Circular) { n->next = n; d.push_back(Delta::edgeAdd(n->id, n->id, Slot::Next)); }
        ev(OpType::PointerMove, d, {rt(n, Role::Active)}, "The list was empty: HEAD and TAIL both point to the new node.", 0);
        size_++;
    }

    // ------------------------------------------------------------ insertion
    void insertBegin(int value) {
        Node* n = makeNode(value);
        if (!head_) { linkFirstNode(n); return; }
        n->next = head_;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(n->id, head_->id, Slot::Next)}, {rt(n, Role::Active), rt(head_, Role::Path)}, "new.next = HEAD: the new node now points to " + v(head_) + ".", 0);
        if (type_ == Type::Doubly) {
            head_->prev = n;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(head_->id, n->id, Slot::Prev)}, {rt(head_, Role::Path)}, "HEAD.prev = new: the old first node points back to the new node.", 0);
        }
        if (type_ == Type::Circular) {
            tail_->next = n;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(tail_->id, n->id, Slot::Next)}, {rt(tail_, Role::Path)}, "tail.next = new: the last node now points to the new first node.", 0);
        }
        head_ = n; size_++;
        ev(OpType::PointerMove, {ptrD("HEAD", n)}, {rt(n, Role::Current)}, "HEAD = new.", 0);
    }

    void insertEnd(int value) {
        if (!head_) { Node* n = makeNode(value); linkFirstNode(n); return; }
        Node* last;
        if (type_ == Type::Doubly) {
            last = tail_;
            ev(OpType::PointerMove, {ptrD("cur", last)}, {rt(last, Role::Current)}, "A doubly linked list keeps TAIL, so we can jump straight to the last node (" + v(last) + ").", 1);
        } else {
            last = walk(size_ - 1);
        }
        Node* n = makeNode(value);
        last->next = n;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(last->id, n->id, Slot::Next)}, {rt(last, Role::Path), rt(n, Role::Active)}, "last.next = new: the next pointer of " + v(last) + " is changed to " + v(n) + ".", 1);
        if (type_ == Type::Doubly) {
            n->prev = last;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(n->id, last->id, Slot::Prev)}, {rt(n, Role::Active)}, "new.prev = last.", 1);
        }
        if (type_ == Type::Circular) {
            n->next = head_;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(n->id, head_->id, Slot::Next)}, {rt(n, Role::Active)}, "new.next = HEAD: the list stays circular.", 1);
        }
        tail_ = n; size_++;
        ev(OpType::PointerMove, {ptrD("TAIL", n)}, {rt(n, Role::Current)}, "TAIL = new.", 1);
    }

    void insertPos(int pos, int value) {
        if (pos < 0 || pos > size_)
            throw InvalidOperation("Invalid position. Please enter a position between 0 and " + to_string(size_) + ".");
        if (pos == 0) { insertBegin(value); return; }
        if (pos == size_) { insertEnd(value); return; }
        Node* prev = walk(pos - 1);
        Node* n = makeNode(value);
        Node* nxt = prev->next;
        n->next = nxt;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(n->id, nxt->id, Slot::Next)}, {rt(n, Role::Active), rt(nxt, Role::Path)}, "new.next = prev.next: the new node points to " + v(nxt) + ".", 2);
        if (type_ == Type::Doubly) {
            n->prev = prev; nxt->prev = n;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(n->id, prev->id, Slot::Prev), Delta::edgeAdd(nxt->id, n->id, Slot::Prev)}, {rt(n, Role::Active)}, "Set new.prev = prev and next.prev = new.", 2);
        }
        prev->next = n;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(prev->id, n->id, Slot::Next)}, {rt(prev, Role::Path), rt(n, Role::Active)},
           "Node " + v(n) + " is inserted between " + v(prev) + " and " + v(nxt) + ". The next pointer of " + v(prev) + " is changed to " + v(n) + ".", 2);
        size_++;
    }

    // ------------------------------------------------------------- deletion
    void requireNonEmpty() { if (!head_) throw InvalidOperation("Underflow: the list is empty, there is nothing to delete."); }

    void removeOnlyNode() {
        Node* d = head_;
        ev(OpType::NodeDelete, {Delta::destroy(d->id), ptrD("HEAD", nullptr), ptrD("TAIL", nullptr)}, {}, "The only node is removed; the list is empty again.", 3);
        freeNode(d); head_ = tail_ = nullptr; size_ = 0;
    }

    void deleteBegin() {
        requireNonEmpty();
        Node* d = head_;
        ev(OpType::Highlight, {ptrD("del", d)}, {rt(d, Role::Candidate)}, "Delete the first node (value " + v(d) + ").", 3);
        if (size_ == 1) { removeOnlyNode(); return; }
        Node* nh = d->next;
        if (type_ == Type::Circular) {
            tail_->next = nh;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(tail_->id, nh->id, Slot::Next)}, {rt(tail_, Role::Path)}, "tail.next = second node: the last node skips the old head.", 3);
        }
        if (type_ == Type::Doubly) {
            nh->prev = nullptr;
            ev(OpType::EdgeRemove, {Delta::edgeRemove(nh->id, d->id, Slot::Prev)}, {rt(nh, Role::Path)}, "second.prev = NULL.", 3);
        }
        head_ = nh;
        ev(OpType::PointerMove, {ptrD("HEAD", nh)}, {rt(nh, Role::Current)}, "HEAD = HEAD.next (" + v(nh) + ").", 3);
        ev(OpType::NodeDelete, {Delta::destroy(d->id)}, {}, "The old first node is freed.", 3);
        freeNode(d); size_--;
    }

    void deleteEnd() {
        requireNonEmpty();
        if (size_ == 1) {
            ev(OpType::Highlight, {ptrD("del", head_)}, {rt(head_, Role::Candidate)}, "Delete the only node.", 4);
            removeOnlyNode(); return;
        }
        Node *prev, *d;
        if (type_ == Type::Doubly) {
            d = tail_; prev = d->prev;
            ev(OpType::PointerMove, {ptrD("cur", prev), ptrD("del", d)}, {rt(prev, Role::Current), rt(d, Role::Candidate)}, "TAIL.prev gives the second last node directly.", 4);
        } else {
            prev = walk(size_ - 2);
            d = prev->next;
            ev(OpType::Highlight, {ptrD("del", d)}, {rt(d, Role::Candidate)}, "The node after cur is the last node (value " + v(d) + ").", 4);
        }
        if (type_ == Type::Circular) {
            prev->next = head_;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(prev->id, head_->id, Slot::Next)}, {rt(prev, Role::Path)}, "prev.next = HEAD: the new last node closes the circle.", 4);
        } else {
            prev->next = nullptr;
            ev(OpType::EdgeRemove, {Delta::edgeRemove(prev->id, d->id, Slot::Next)}, {rt(prev, Role::Path)}, "prev.next = NULL.", 4);
        }
        tail_ = prev;
        ev(OpType::PointerMove, {ptrD("TAIL", prev)}, {rt(prev, Role::Current)}, "TAIL = prev.", 4);
        ev(OpType::NodeDelete, {Delta::destroy(d->id)}, {}, "The old last node is freed.", 4);
        freeNode(d); size_--;
    }

    void deletePos(int pos) {
        requireNonEmpty();
        if (pos < 0 || pos >= size_)
            throw InvalidOperation("Invalid position. Please enter a position between 0 and " + to_string(size_ - 1) + ".");
        if (pos == 0) { deleteBegin(); return; }
        if (pos == size_ - 1) { deleteEnd(); return; }
        Node* prev = walk(pos - 1);
        Node* d = prev->next;
        Node* nxt = d->next;
        ev(OpType::Highlight, {ptrD("del", d)}, {rt(d, Role::Candidate)}, "The node after cur (value " + v(d) + ") will be deleted.", 5);
        prev->next = nxt;
        ev(OpType::EdgeAdd, {Delta::edgeAdd(prev->id, nxt->id, Slot::Next)}, {rt(prev, Role::Path), rt(nxt, Role::Path)}, "prev.next = del.next: " + v(prev) + " now points to " + v(nxt) + ".", 5);
        if (type_ == Type::Doubly) {
            nxt->prev = prev;
            ev(OpType::EdgeAdd, {Delta::edgeAdd(nxt->id, prev->id, Slot::Prev)}, {rt(nxt, Role::Path)}, "next.prev = prev.", 5);
        }
        ev(OpType::NodeDelete, {Delta::destroy(d->id)}, {}, "The node is freed.", 5);
        freeNode(d); size_--;
    }

    // ------------------------------------------------------- search / traverse
    void search(int target) {
        if (!head_) throw InvalidOperation("Cannot search: the list is empty.");
        Node* c = head_;
        for (int i = 0; i < size_; i++, c = c->next) {
            ev(OpType::VisitNode, {ptrD("cur", c)}, {rt(c, Role::Current)}, "cur is at node " + v(c) + " (position " + to_string(i) + ").", 6);
            bool eq = c->value == target;
            ev(OpType::Compare, {}, {rt(c, Role::Compared)}, "Compare " + v(c) + " with " + to_string(target) + (eq ? ": equal." : ": not equal."), 6);
            if (eq) {
                ev(OpType::Highlight, {ptrD("cur", nullptr)}, {rt(c, Role::Found)}, "Found " + to_string(target) + " at position " + to_string(i) + ".", 6);
                return;
            }
        }
        ev(OpType::Highlight, {ptrD("cur", nullptr)}, {}, to_string(target) + " is not in the list" + (type_ == Type::Circular ? " (we stopped after one full circle)." : " (cur reached NULL)."), 6);
    }

    void traverse() {
        if (!head_) throw InvalidOperation("Cannot traverse: the list is empty.");
        Node* c = head_;
        string order;
        for (int i = 0; i < size_; i++, c = c->next) {
            order += (i ? " -> " : "") + v(c);
            ev(OpType::VisitNode, {ptrD("cur", c)}, {rt(c, Role::Visited)}, "Visit node " + v(c) + " (position " + to_string(i) + "). Order so far: " + order, 7);
        }
        ev(OpType::Highlight, {ptrD("cur", nullptr)}, {}, "Traversal complete: " + order + (type_ == Type::Circular ? " -> (back to HEAD)" : " -> NULL"), 7);
    }

    Type type_;
    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    vector<unique_ptr<Node>> pool_;
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    int size_ = 0;
    int nextId_ = 0;
};

inline void registerLists(AlgorithmCatalog& cat) {
    for (auto t : {LinkedListAlgorithm::Type::Singly, LinkedListAlgorithm::Type::Doubly, LinkedListAlgorithm::Type::Circular}) {
        CatalogEntry e;
        LinkedListAlgorithm probe(t);
        e.id = probe.info().id; e.category = "Linked Lists"; e.name = probe.info().name;
        e.input = InputKind::Structure; e.structure = StructureKind::List; e.ops = LinkedListAlgorithm::ops();
        e.make = [t] { return unique_ptr<Algorithm>(new LinkedListAlgorithm(t)); };
        cat.add(e);
    }
}

#endif
