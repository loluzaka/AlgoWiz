#ifndef EXAMPLEPLUGINS_H
#define EXAMPLEPLUGINS_H

// Three small plugins that show how a student adds their own work:
//   GnomeSortPlugin     - a custom ALGORITHM using the easy ArrayTrace-style helpers
//   CocktailSortPlugin  - a second algorithm, also shipped as a loadable library (ExampleDll.cpp)
//   DequePlugin         - a custom DATA STRUCTURE (double-ended queue) mapped onto the array renderer
// They are registered like any third-party plugin; nothing in the engine knows they exist.

#include <string>
#include <vector>

#include "../PluginRegistry.h"

using namespace std;

class GnomeSortPlugin : public CustomAlgorithm {
public:
    PluginMetadata metadata() const override {
        PluginMetadata m;
        m.id = "gnome_sort"; m.name = "Gnome Sort (plugin)"; m.author = "Algo Visualizer team"; m.version = "1.0";
        m.description = "Walks forward while neighbours are in order and steps back (swapping) when they are not, like a garden gnome sorting flower pots.";
        m.kind = PluginKind::Algorithm; m.renders = StructureKind::Array; m.inputSchema = "array";
        m.best = "O(n)"; m.average = "O(n^2)"; m.worst = "O(n^2)"; m.space = "O(1)";
        m.pseudocode = {"i = 0", "while i < n:", "  if i == 0 or a[i-1] <= a[i]: i++", "  else: swap(a[i-1], a[i]); i--"};
        return m;
    }
    void execute(const Input& in, PluginContext& ctx) override {
        ctx.setArray(in.values);
        int n = ctx.size(), i = 0;
        while (i < n) {
            if (i == 0) { i++; continue; }
            bool ordered = in.descending ? ctx.get(i - 1) >= ctx.get(i) : ctx.get(i - 1) <= ctx.get(i);
            ctx.compare(i - 1, i, "Compare " + to_string(ctx.get(i - 1)) + " and " + to_string(ctx.get(i)) + (ordered ? ": in order, step forward." : ": out of order, swap and step back."), 2);
            if (ordered) i++;
            else { ctx.swap(i - 1, i, "Swap and step back.", 3); i--; }
        }
        vector<int> all;
        for (int k = 0; k < n; k++) all.push_back(k);
        if (n > 0) ctx.mark(all, Role::Sorted, "Gnome sort finished.", 1);
    }
};

class CocktailSortPlugin : public CustomAlgorithm {
public:
    PluginMetadata metadata() const override {
        PluginMetadata m;
        m.id = "cocktail_sort"; m.name = "Cocktail Shaker Sort (plugin)"; m.author = "Algo Visualizer team"; m.version = "1.0";
        m.description = "Bubble sort that alternates direction: a forward pass moves the largest value to the right end, a backward pass moves the smallest to the left end.";
        m.kind = PluginKind::Algorithm; m.renders = StructureKind::Array; m.inputSchema = "array";
        m.best = "O(n)"; m.average = "O(n^2)"; m.worst = "O(n^2)"; m.space = "O(1)";
        m.pseudocode = {"repeat:", "  forward pass: bubble the largest to the right", "  backward pass: bubble the smallest to the left", "until no swap happened"};
        return m;
    }
    void execute(const Input& in, PluginContext& ctx) override {
        ctx.setArray(in.values);
        int lo = 0, hi = ctx.size() - 1;
        bool swapped = true;
        while (swapped && lo < hi) {
            swapped = false;
            for (int i = lo; i < hi; i++) {
                bool out = ctx.get(i) > ctx.get(i + 1);
                ctx.compare(i, i + 1, "Forward pass: compare " + to_string(ctx.get(i)) + " and " + to_string(ctx.get(i + 1)) + ".", 1);
                if (out) { ctx.swap(i, i + 1, "Swap.", 1); swapped = true; }
            }
            ctx.mark({hi}, Role::Sorted, "Largest value of this pass is in place.", 1);
            hi--;
            for (int i = hi; i > lo; i--) {
                bool out = ctx.get(i - 1) > ctx.get(i);
                ctx.compare(i - 1, i, "Backward pass: compare " + to_string(ctx.get(i - 1)) + " and " + to_string(ctx.get(i)) + ".", 2);
                if (out) { ctx.swap(i - 1, i, "Swap.", 2); swapped = true; }
            }
            ctx.mark({lo}, Role::Sorted, "Smallest value of this pass is in place.", 2);
            lo++;
        }
        vector<int> all;
        for (int k = 0; k < ctx.size(); k++) all.push_back(k);
        if (ctx.size() > 0) ctx.mark(all, Role::Sorted, "Cocktail sort finished.", 3);
    }
};

// A deque stored in an array: cells shift on push_front / pop_front so the picture shows the cost.
class DequePlugin : public CustomStructure {
public:
    PluginMetadata metadata() const override {
        PluginMetadata m;
        m.id = "deque"; m.name = "Deque (plugin)"; m.author = "Algo Visualizer team"; m.version = "1.0";
        m.description = "Double-ended queue: insert and remove at BOTH ends. This array version shifts cells when the front changes.";
        m.kind = PluginKind::Structure; m.renders = StructureKind::Array;
        m.best = "O(1) back"; m.average = "O(n) front"; m.worst = "O(n)"; m.space = "O(n)";
        m.pseudocode = {"push_back(x): a[n] = x", "push_front(x): shift right; a[0] = x", "pop_back(): n--", "pop_front(): shift left; n--"};
        m.ops = {{"push_front", "Push front", {"value"}}, {"push_back", "Push back", {"value"}}, {"pop_front", "Pop front", {}}, {"pop_back", "Pop back", {}}};
        return m;
    }
    void reset(const Input&, PluginContext& ctx) override { d_.clear(); ctx.setup({Delta::resize(CAPACITY, 0)}); }
    void addInitial(int v, PluginContext& ctx) override { Command c; c.op = "push_back"; c.a = v; apply(c, ctx); }
    void apply(const Command& c, PluginContext& ctx) override {
        int n = (int)d_.size();
        if (c.op == "push_back" || c.op == "push_front") {
            if (n >= CAPACITY) throw InvalidOperation("Overflow: the deque is full (" + to_string(CAPACITY) + " cells).");
            bool front = c.op == "push_front";
            if (front) {
                d_.insert(d_.begin(), c.a);
                for (int i = n; i >= 1; i--)
                    ctx.emit(OpType::Assign, {Delta::value(i, d_[i], 0)}, {tag(i, Role::Moved)}, "Shift " + to_string(d_[i]) + " one cell to the right.", 2);
            } else d_.push_back(c.a);
            int at = front ? 0 : n;
            ctx.emit(OpType::Insert, {Delta::value(at, c.a, 0)}, {tag(at, Role::Found)}, "Store " + to_string(c.a) + " at the " + (front ? "front" : "back") + " (index " + to_string(at) + ").", front ? 1 : 0);
        } else if (c.op == "pop_back" || c.op == "pop_front") {
            if (n == 0) throw InvalidOperation("Underflow: the deque is empty.");
            bool front = c.op == "pop_front";
            int removed = front ? d_.front() : d_.back();
            ctx.emit(OpType::Highlight, {}, {tag(front ? 0 : n - 1, Role::Candidate)}, "Remove " + to_string(removed) + " from the " + (front ? "front" : "back") + ".", front ? 3 : 2);
            if (front) {
                d_.erase(d_.begin());
                for (int i = 0; i < n - 1; i++)
                    ctx.emit(OpType::Assign, {Delta::value(i, d_[i], 0)}, {tag(i, Role::Moved)}, "Shift " + to_string(d_[i]) + " one cell to the left.", 3);
            } else d_.pop_back();
            ctx.emit(OpType::Delete, {Delta::destroy(n - 1, 0)}, {}, "Cell " + to_string(n - 1) + " is now empty. Removed " + to_string(removed) + ".", front ? 3 : 2);
        } else {
            throw InvalidOperation("Unknown deque operation: " + c.op);
        }
        vector<Delta> p = {d_.empty() ? Delta::pointer("FRONT", -1, 0) : Delta::pointer("FRONT", 0, 0), d_.empty() ? Delta::pointer("BACK", -1, 0) : Delta::pointer("BACK", (int)d_.size() - 1, 0)};
        ctx.emit(OpType::PointerMove, p, {}, "FRONT = " + string(d_.empty() ? "-" : "0") + ", BACK = " + (d_.empty() ? string("-") : to_string(d_.size() - 1)) + ".");
    }

private:
    static const int CAPACITY = 10;
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    vector<int> d_;
};

inline void registerExamplePlugins(PluginRegistry& reg) {
    reg.registerAlgorithm([] { return unique_ptr<CustomAlgorithm>(new GnomeSortPlugin()); });
    reg.registerStructure([] { return unique_ptr<CustomStructure>(new DequePlugin()); });
}

#endif
