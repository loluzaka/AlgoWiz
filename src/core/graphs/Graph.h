#ifndef GRAPH_H
#define GRAPH_H

// Weighted graph (directed or undirected) with BFS and DFS.
//
// The model keeps ONE source of truth (an adjacency map); the adjacency-list and
// adjacency-matrix pictures are two views of it that the GraphRenderer draws.
// Picture conventions (container 0): vertex = node id with a letter as its text;
// edge = EdgeAdd with a weight (an undirected edge is stored as two opposite edges).
// Metadata pointers: "directed" (1/0) and "from"/"to" (the edge currently explored).
// Container 1 shows the BFS queue / DFS recursion stack as a small array.

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

class GraphAlgorithm : public StructureAlgorithm {
public:
    static const int MAX_VERTICES = 10;

    GraphAlgorithm() {
        info_.id = "graph"; info_.name = "Graph (BFS / DFS)"; info_.category = "Graphs";
        info_.best = "O(V + E) traversal"; info_.average = "O(V + E)"; info_.worst = "O(V + E)"; info_.space = "O(V + E) list / O(V^2) matrix";
        info_.description = "Vertices connected by weighted edges. Shown first as an adjacency matrix, with drawing and adjacency-list views also available. BFS explores level by level with a queue; DFS goes deep first using a stack.";
        info_.pseudocode = {"BFS(start): queue = [start]; discovered[start] = true", "  while queue not empty:", "    u = dequeue(); visit(u)",
                            "    for each neighbour v of u: if not discovered: discovered[v] = true; enqueue(v)",
                            "DFS(u): visit(u); mark visited; push u", "  for each neighbour v of u: if not visited: DFS(v)", "  pop u (backtrack)"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    string validate(const Input& in) const override {
        if (in.vertexCount < 0 || in.vertexCount > MAX_VERTICES) return "A graph may have at most " + to_string(MAX_VERTICES) + " vertices.";
        return "";
    }
    static vector<OpSpec> ops() {
        return {{"add_vertex", "Add vertex", {}}, {"delete_vertex", "Delete vertex", {"vertex number (A=0, B=1 ...)"}},
                {"add_edge", "Add edge", {"from vertex", "to vertex", "weight"}}, {"delete_edge", "Delete edge", {"from vertex", "to vertex"}},
                {"set_weight", "Set edge weight", {"from vertex", "to vertex", "new weight"}},
                {"bfs", "Breadth-first search", {"start vertex"}}, {"dfs", "Depth-first search", {"start vertex"}}};
    }
    static string vname(int id) { return id >= 0 && id < 26 ? string(1, (char)('A' + id)) : "V" + to_string(id); }

    // ---- read access for tests ----
    vector<int> vertices() const { return vector<int>(verts_.begin(), verts_.end()); }
    bool hasEdge(int u, int v) const { auto it = adj_.find(u); return it != adj_.end() && it->second.count(v); }
    int weight(int u, int v) const { return adj_.at(u).at(v); }
    int edgeCount() const { int n = 0; for (const auto& kv : adj_) n += (int)kv.second.size(); return directed_ ? n : n / 2; }
    const vector<int>& lastOrder() const { return order_; }

protected:
    StructureKind structureKind() const override { return StructureKind::Graph; }
    vector<string> containerNames() const override { return {"Graph", "Queue / stack"}; }

    void begin(const Input& in, Recorder& rec) override {
        rec_ = &rec; verts_.clear(); adj_.clear(); marked_.clear(); order_.clear(); directed_ = in.directed; nextId_ = 0;
        rec.setup({Delta::pointer("directed", directed_ ? 1 : 0, 0), Delta::resize(0, 1)});
        for (int i = 0; i < in.vertexCount; i++) createVertex();
        for (size_t i = 0; i + 2 < in.values.size(); i += 3) {
            int u = in.values[i], v = in.values[i + 1], w = in.values[i + 2];
            if (verts_.count(u) && verts_.count(v) && u != v && !hasEdge(u, v)) addEdgeModel(u, v, w);
        }
    }
    void addInitial(int, Recorder&) override {}      // the initial edge list is read in begin()

    void cleanup(Recorder& rec) override {
        rec_ = &rec;
        vector<Delta> d;
        for (int id : marked_) d.push_back(Delta::mark(id, Role::Default));
        marked_.clear();
        d.push_back(Delta::pointer("from", -1, 0)); d.push_back(Delta::pointer("to", -1, 0)); d.push_back(Delta::clear(1));
        d.push_back(Delta::resize(0, 1));
        rec.emit(OpType::Unmark, d, {}, "Clear the highlights left by the previous operation.");
    }

    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "add_vertex") {
            if ((int)verts_.size() >= MAX_VERTICES) throw InvalidOperation("A graph may have at most " + to_string(MAX_VERTICES) + " vertices in this view.");
            createVertex();
        } else if (c.op == "delete_vertex") {
            needVertex(c.a);
            deleteVertex(c.a);
        } else if (c.op == "add_edge") {
            needVertex(c.a); needVertex(c.b);
            if (c.a == c.b) throw InvalidOperation("Self-loops are not supported. Choose two different vertices.");
            if (hasEdge(c.a, c.b)) throw InvalidOperation("The edge " + vname(c.a) + (directed_ ? " -> " : " - ") + vname(c.b) + " already exists. Use 'Set edge weight' to change its weight.");
            addEdgeVisible(c.a, c.b, c.c);
        } else if (c.op == "delete_edge") {
            needVertex(c.a); needVertex(c.b);
            if (!hasEdge(c.a, c.b)) throw InvalidOperation("There is no edge between " + vname(c.a) + " and " + vname(c.b) + ".");
            deleteEdge(c.a, c.b);
        } else if (c.op == "set_weight") {
            needVertex(c.a); needVertex(c.b);
            if (!hasEdge(c.a, c.b)) throw InvalidOperation("There is no edge between " + vname(c.a) + " and " + vname(c.b) + ".");
            setWeight(c.a, c.b, c.c);
        } else if (c.op == "bfs") { needVertex(c.a); bfs(c.a); }
        else if (c.op == "dfs") { needVertex(c.a); dfs(c.a); }
        else throw InvalidOperation("Unknown graph operation: " + c.op);
    }

private:
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    void ev(OpType op, vector<Delta> d, vector<RoleTag> r, const string& note, int line = -1) { rec_->emit(op, std::move(d), std::move(r), note, line); }
    void needVertex(int v) {
        if (!verts_.count(v)) throw InvalidOperation("Vertex " + (v >= 0 ? vname(v) : to_string(v)) + " does not exist.");
    }
    string edgeText(int u, int v) const { return vname(u) + (directed_ ? " -> " : " - ") + vname(v); }

    // ------------------------------------------------------------ editing
    void createVertex() {
        int id = nextId_++;
        verts_.insert(id); adj_[id];
        ev(OpType::NodeCreate, {Delta::createText(id, vname(id), 0)}, {tag(id, Role::Active)}, "Add vertex " + vname(id) + ".", 0);
    }
    void addEdgeModel(int u, int v, int w) {
        adj_[u][v] = w;
        vector<Delta> d = {Delta::edgeAdd(u, v, Slot::None, w, 0)};
        if (!directed_) { adj_[v][u] = w; d.push_back(Delta::edgeAdd(v, u, Slot::None, w, 0)); }
        ev(OpType::EdgeAdd, d, {}, "");
    }
    void addEdgeVisible(int u, int v, int w) {
        adj_[u][v] = w;
        vector<Delta> d = {Delta::edgeAdd(u, v, Slot::None, w, 0)};
        if (!directed_) { adj_[v][u] = w; d.push_back(Delta::edgeAdd(v, u, Slot::None, w, 0)); }
        ev(OpType::EdgeAdd, d, {tag(u, Role::Active), tag(v, Role::Active)}, "Add the " + string(directed_ ? "directed " : "undirected ") + "edge " + edgeText(u, v) + " with weight " + to_string(w) + ".", 0);
    }
    void deleteEdge(int u, int v) {
        vector<Delta> d = {Delta::edgeRemove(u, v, Slot::None, 0)};
        adj_[u].erase(v);
        if (!directed_) { adj_[v].erase(u); d.push_back(Delta::edgeRemove(v, u, Slot::None, 0)); }
        ev(OpType::EdgeRemove, d, {tag(u, Role::Candidate), tag(v, Role::Candidate)}, "Delete the edge " + edgeText(u, v) + ".", 0);
    }
    void setWeight(int u, int v, int w) {
        adj_[u][v] = w;
        vector<Delta> d = {Delta::edgeAdd(u, v, Slot::None, w, 0)};
        if (!directed_) { adj_[v][u] = w; d.push_back(Delta::edgeAdd(v, u, Slot::None, w, 0)); }
        ev(OpType::EdgeAdd, d, {tag(u, Role::Active), tag(v, Role::Active)}, "Set the weight of " + edgeText(u, v) + " to " + to_string(w) + ".", 0);
    }
    void deleteVertex(int x) {
        ev(OpType::Highlight, {}, {tag(x, Role::Candidate)}, "Delete vertex " + vname(x) + " and every edge touching it.", 0);
        vector<int> others;
        for (int u : verts_) if (u != x && (hasEdge(u, x) || hasEdge(x, u))) others.push_back(u);
        for (int u : others) {
            vector<Delta> d;
            if (hasEdge(u, x)) { adj_[u].erase(x); d.push_back(Delta::edgeRemove(u, x, Slot::None, 0)); }
            if (hasEdge(x, u)) { adj_[x].erase(u); d.push_back(Delta::edgeRemove(x, u, Slot::None, 0)); }
            ev(OpType::EdgeRemove, d, {tag(u, Role::Path), tag(x, Role::Candidate)}, "Remove the edge between " + vname(u) + " and " + vname(x) + ".", 0);
        }
        verts_.erase(x); adj_.erase(x);
        ev(OpType::NodeDelete, {Delta::destroy(x, 0)}, {}, "Vertex " + vname(x) + " is removed.", 0);
    }

    // --------------------------------------------------------- aux container
    void showAux(const vector<int>& items, OpType op, const string& note, vector<RoleTag> roles = {}) {
        vector<Delta> d = {Delta::clear(1), Delta::resize((int)items.size(), 1)};
        for (size_t i = 0; i < items.size(); i++) d.push_back(Delta::createText((int)i, vname(items[i]), 1));
        ev(op, d, std::move(roles), note, 1);
    }
    void exploring(int u, int v, const string& note, vector<RoleTag> roles, int line) {
        ev(OpType::VisitEdge, {Delta::pointer("from", u, 0), Delta::pointer("to", v, 0)}, std::move(roles), note, line);
    }
    void stopExploring(int u) {
        ev(OpType::PointerMove, {Delta::pointer("from", -1, 0), Delta::pointer("to", -1, 0)}, {tag(u, Role::Current)},
           "All neighbors of " + vname(u) + " have been examined.", 0);
    }
    string orderText() const {
        string s;
        for (size_t i = 0; i < order_.size(); i++) s += (i ? " -> " : "") + vname(order_[i]);
        return s;
    }

    // ---------------------------------------------------------------- BFS
    void bfs(int start) {
        order_.clear();
        vector<int> queue = {start};
        set<int> seen = {start};
        marked_.insert(start);
        ev(OpType::Mark, {Delta::mark(start, Role::Queued, 0)}, {tag(start, Role::Current)}, "BFS from " + vname(start) + ": put it in the queue and mark it discovered.", 0);
        showAux(queue, OpType::Insert, "Enqueue " + vname(start) + ".");
        while (!queue.empty()) {
            int u = queue.front();
            queue.erase(queue.begin());
            showAux(queue, OpType::Delete, "Dequeue " + vname(u) + ".", {tag(u, Role::Current)});
            order_.push_back(u);
            marked_.insert(u);
            ev(OpType::VisitNode, {Delta::mark(u, Role::Visited, 0)}, {tag(u, Role::Current)},
               "Vertex " + vname(u) + " is removed from the queue and its unvisited neighbors are examined. Order so far: " + orderText(), 2);
            for (const auto& nb : adj_[u]) {
                int v = nb.first;
                bool fresh = !seen.count(v);
                exploring(u, v, "Explore edge " + edgeText(u, v) + (fresh ? ": " + vname(v) + " is new." : ": " + vname(v) + " was already discovered, skip."),
                          {tag(u, Role::Current), tag(v, fresh ? Role::Frontier : Role::Visited)}, 3);
                if (fresh) {
                    seen.insert(v); queue.push_back(v);
                    marked_.insert(v);
                    ev(OpType::Mark, {Delta::mark(v, Role::Queued, 0)}, {tag(v, Role::Frontier)}, "Mark " + vname(v) + " as discovered.", 3);
                    showAux(queue, OpType::Insert, "Enqueue " + vname(v) + ".");
                }
            }
            stopExploring(u);
        }
        ev(OpType::PointerMove, {Delta::pointer("from", -1, 0)}, {}, "BFS finished. Traversal order: " + orderText(), 1);
    }

    // ---------------------------------------------------------------- DFS
    void dfs(int start) {
        order_.clear();
        set<int> seen;
        vector<int> stack;
        ev(OpType::Note, {}, {tag(start, Role::Current)}, "DFS from " + vname(start) + ": go as deep as possible, then backtrack.", 4);
        dfsVisit(start, seen, stack);
        ev(OpType::PointerMove, {Delta::pointer("from", -1, 0)}, {}, "DFS finished. Traversal order: " + orderText(), 4);
    }
    void dfsVisit(int u, set<int>& seen, vector<int>& stack) {
        seen.insert(u); stack.push_back(u); order_.push_back(u); marked_.insert(u);
        showAux(stack, OpType::Insert, "Push " + vname(u) + " on the recursion stack.", {tag(u, Role::Current)});
        ev(OpType::VisitNode, {Delta::mark(u, Role::Visited, 0)}, {tag(u, Role::Current)}, "Visit " + vname(u) + ". Order so far: " + orderText(), 4);
        for (const auto& nb : adj_[u]) {
            int v = nb.first;
            bool fresh = !seen.count(v);
            exploring(u, v, "Explore edge " + edgeText(u, v) + (fresh ? ": " + vname(v) + " is unvisited, go deeper." : ": " + vname(v) + " was already visited."),
                      {tag(u, Role::Current), tag(v, fresh ? Role::Frontier : Role::Visited)}, 5);
            if (fresh) dfsVisit(v, seen, stack);
        }
        stopExploring(u);
        stack.pop_back();
        showAux(stack, OpType::Delete, "All neighbors of " + vname(u) + " are done: pop it and backtrack.", stack.empty() ? vector<RoleTag>{} : vector<RoleTag>{tag(stack.back(), Role::Current)});
    }

    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    set<int> verts_;
    map<int, map<int, int>> adj_;
    set<int> marked_;
    vector<int> order_;
    bool directed_ = false;
    int nextId_ = 0;
};

inline void registerGraphs(AlgorithmCatalog& cat) {
    CatalogEntry e;
    GraphAlgorithm probe;
    e.id = probe.info().id; e.category = "Graphs"; e.name = probe.info().name;
    e.input = InputKind::Structure; e.structure = StructureKind::Graph; e.ops = GraphAlgorithm::ops();
    e.make = [] { return unique_ptr<Algorithm>(new GraphAlgorithm()); };
    cat.add(e);
}

#endif
