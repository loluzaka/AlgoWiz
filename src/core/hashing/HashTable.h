#ifndef HASHTABLE_H
#define HASHTABLE_H

// Hash table with Linear probing, Quadratic probing and Chaining.  hash(key) = key mod m.
// Every probe is its own step; collisions, the probe sequence, search paths and
// deletion (tombstones for open addressing, unlinking for chaining) are all visible.
//
// Layout in the picture (container 0): slot i is node id i (i < tableSize).
// Chaining stores each chain node with id >= CHAIN_BASE in the SAME container and links
// slot -> first node -> next node with Slot::Next edges.

#include <string>
#include <vector>

#include "../../engine/StructureAlgorithm.h"
#include "../AlgorithmCatalog.h"

using namespace std;

enum class HashMethod { Linear, Quadratic, Chaining };

class HashTableAlgorithm : public StructureAlgorithm {
public:
    static const int CHAIN_BASE = 100;

    explicit HashTableAlgorithm(HashMethod m) : method_(m) {
        info_.category = "Hashing";
        info_.best = "O(1)"; info_.average = "O(1)"; info_.worst = "O(n)"; info_.space = "O(m + n)";
        switch (m) {
        case HashMethod::Linear:    info_.id = "hash_linear";    info_.name = "Hash Table - Linear Probing";
            info_.description = "Open addressing: on a collision try the next slot (h+1, h+2, ...). Deleted slots become tombstones."; break;
        case HashMethod::Quadratic: info_.id = "hash_quadratic"; info_.name = "Hash Table - Quadratic Probing";
            info_.description = "Open addressing: on a collision try h+1^2, h+2^2, ... which spreads clusters. May miss free slots when m is not prime."; break;
        case HashMethod::Chaining:  info_.id = "hash_chaining";  info_.name = "Hash Table - Chaining";
            info_.description = "Each slot holds a linked chain of the keys that hash to it, so collisions simply lengthen the chain."; break;
        }
        info_.pseudocode = {"h = key mod m", "i = 0; idx = (h + i) mod m   [quadratic: (h + i*i) mod m]", "while slot idx is occupied: collision, i++",
                            "store key at idx", "search: follow the same probe sequence until key or an empty slot", "delete: find key, mark slot as DELETED (tombstone)",
                            "chaining: append key to chain[h]; search/delete walk chain[h]"};
    }
    const AlgorithmInfo& info() const override { return info_; }
    string validate(const Input& in) const override {
        if (in.tableSize < 3 || in.tableSize > 13) return "Table size must be between 3 and 13.";
        return "";
    }
    static vector<OpSpec> ops() { return {{"insert", "Insert key", {"key"}}, {"search", "Search key", {"key"}}, {"delete", "Delete key", {"key"}}}; }

    vector<int> keysInTable() const {            // for tests: all stored keys, slot order (chains in order)
        vector<int> r;
        if (method_ == HashMethod::Chaining) { for (const auto& c : chains_) for (const auto& n : c) r.push_back(n.second); }
        else for (int i = 0; i < m_; i++) if (state_[i] == FULL) r.push_back(keys_[i]);
        return r;
    }

protected:
    StructureKind structureKind() const override { return StructureKind::Hash; }
    vector<string> containerNames() const override { return {"Hash table"}; }

    void begin(const Input& in, Recorder& rec) override {
        rec_ = &rec; m_ = in.tableSize; keys_.assign(m_, 0); state_.assign(m_, EMPTY); chains_.assign(m_, {}); nextChain_ = CHAIN_BASE;
        rec.setup({Delta::resize(m_, 0)});
    }
    void addInitial(int v, Recorder&) override { insertKey(v); }
    void execute(const Command& c, Recorder& rec) override {
        rec_ = &rec;
        if (c.op == "insert") insertKey(c.a);
        else if (c.op == "search") searchKey(c.a);
        else if (c.op == "delete") deleteKey(c.a);
        else throw InvalidOperation("Unknown hash operation: " + c.op);
    }

private:
    enum { EMPTY = 0, FULL = 1, TOMB = 2 };
    static RoleTag tag(int i, Role r) { RoleTag t; t.target_id = i; t.role = r; return t; }
    void ev(OpType op, vector<Delta> d, vector<RoleTag> r, const string& note, int line = -1) { rec_->emit(op, std::move(d), std::move(r), note, line); }
    int  probeIndex(int h, int i) const { return method_ == HashMethod::Quadratic ? (int)(((long long)h + (long long)i * i) % m_) : (h + i) % m_; }
    int  mod(int key) const { return ((key % m_) + m_) % m_; }

    int emitHash(int key) {
        int h = mod(key);
        ev(OpType::Hash, {}, {tag(h, Role::Active)}, "hash(" + to_string(key) + ") = " + to_string(key) + " mod " + to_string(m_) + " = " + to_string(h) + ".", 0);
        return h;
    }
    void clearProbe(const string& note, vector<RoleTag> roles = {}) { ev(OpType::PointerMove, {Delta::pointer("probe", -1, 0)}, std::move(roles), note); }
    string probeText(int h, int i, int idx) const {
        if (i == 0) return "Probe index " + to_string(idx) + " (the home slot).";
        string f = method_ == HashMethod::Quadratic ? "(" + to_string(h) + " + " + to_string(i) + "^2) mod " + to_string(m_)
                                                    : "(" + to_string(h) + " + " + to_string(i) + ") mod " + to_string(m_);
        return "Probe #" + to_string(i) + ": " + f + " = " + to_string(idx) + ".";
    }

    void insertKey(int key) { if (method_ == HashMethod::Chaining) insertChain(key); else insertOpen(key); }
    void searchKey(int key) { if (method_ == HashMethod::Chaining) searchChain(key, false); else searchOpen(key, false); }
    void deleteKey(int key) { if (method_ == HashMethod::Chaining) searchChain(key, true); else searchOpen(key, true); }

    // ------------------------------------------------------- open addressing
    void insertOpen(int key) {
        int h = emitHash(key);
        for (int i = 0; i < m_; i++) {
            int idx = probeIndex(h, i);
            ev(OpType::Probe, {Delta::pointer("probe", idx, 0)}, {tag(idx, Role::Current)}, probeText(h, i, idx), 1);
            if (state_[idx] == FULL) {
                if (keys_[idx] == key) { clearProbe("Key " + to_string(key) + " is already stored at index " + to_string(idx) + "; duplicates are not inserted.", {tag(idx, Role::Found)}); return; }
                ev(OpType::Collision, {}, {tag(idx, Role::Collision)},
                   "COLLISION: index " + to_string(idx) + " is occupied by " + to_string(keys_[idx]) + ", so " + (method_ == HashMethod::Quadratic ? "quadratic" : "linear") + " probing checks the next slot.", 2);
                continue;
            }
            bool tomb = state_[idx] == TOMB;
            state_[idx] = FULL; keys_[idx] = key;
            ev(OpType::Insert, {Delta::value(idx, key, 0), Delta::pointer("probe", -1, 0)}, {tag(idx, Role::Moved)},
               "Slot " + to_string(idx) + (tomb ? " holds a deleted marker and can be reused" : " is empty") + ": insert " + to_string(key) + " at index " + to_string(idx) + ".", 3);
            return;
        }
        clearProbe("No free slot was found after " + to_string(m_) + " probes: " + to_string(key) + " cannot be inserted.", {});
    }

    void searchOpen(int key, bool remove) {
        int h = emitHash(key);
        for (int i = 0; i < m_; i++) {
            int idx = probeIndex(h, i);
            ev(OpType::Probe, {Delta::pointer("probe", idx, 0)}, {tag(idx, Role::Current)}, probeText(h, i, idx), 4);
            if (state_[idx] == EMPTY) { clearProbe("Slot " + to_string(idx) + " is empty: " + to_string(key) + " is not in the table.", {tag(idx, Role::NotFound)}); return; }
            if (state_[idx] == FULL) {
                bool eq = keys_[idx] == key;
                ev(OpType::Compare, {}, {tag(idx, Role::Compared)}, "Compare " + to_string(keys_[idx]) + " with " + to_string(key) + (eq ? ": found." : ": different, keep probing."), 4);
                if (eq) {
                    if (!remove) { clearProbe("Found " + to_string(key) + " at index " + to_string(idx) + " after " + to_string(i + 1) + " probe(s).", {tag(idx, Role::Found)}); return; }
                    state_[idx] = TOMB;
                    ev(OpType::Delete, {Delta::valueText(idx, "DEL", 0), Delta::pointer("probe", -1, 0)}, {tag(idx, Role::Moved)},
                       "Deleted " + to_string(key) + ": index " + to_string(idx) + " is marked DEL (a tombstone) so later searches keep probing past it.", 5);
                    return;
                }
            }
        }
        clearProbe(to_string(key) + " was not found: every slot on the probe sequence was checked.", {});
    }

    // ------------------------------------------------------------- chaining
    void insertChain(int key) {
        int h = emitHash(key);
        auto& chain = chains_[h];
        ev(OpType::Probe, {Delta::pointer("probe", h, 0)}, {tag(h, Role::Current)}, "Go to slot " + to_string(h) + (chain.empty() ? " (its chain is empty)." : " (its chain already has " + to_string(chain.size()) + " key(s))."), 6);
        int prev = h;
        for (auto& node : chain) {
            ev(OpType::Compare, {}, {tag(node.first, Role::Compared)}, "Compare " + to_string(node.second) + " with " + to_string(key) + (node.second == key ? ": already present." : ": different."), 6);
            if (node.second == key) { clearProbe("Key " + to_string(key) + " is already in chain " + to_string(h) + ".", {tag(node.first, Role::Found)}); return; }
            prev = node.first;
        }
        if (!chain.empty()) ev(OpType::Collision, {}, {tag(h, Role::Collision)}, "COLLISION: slot " + to_string(h) + " already has keys, so the new key is chained after them.", 6);
        int id = nextChain_++;
        chain.push_back({id, key});
        ev(OpType::NodeCreate, {Delta::create(id, key, 0), Delta::edgeAdd(prev, id, Slot::Next, 1, 0), Delta::pointer("probe", -1, 0)}, {tag(id, Role::Moved)},
           "Append " + to_string(key) + " to chain " + to_string(h) + ".", 6);
    }

    void searchChain(int key, bool remove) {
        int h = emitHash(key);
        auto& chain = chains_[h];
        ev(OpType::Probe, {Delta::pointer("probe", h, 0)}, {tag(h, Role::Current)}, "Go to slot " + to_string(h) + " and walk its chain.", 6);
        int prev = h;
        for (size_t i = 0; i < chain.size(); i++) {
            auto node = chain[i];
            ev(OpType::Probe, {}, {tag(node.first, Role::Current)}, "Look at chain node " + to_string(node.second) + ".", 6);
            bool eq = node.second == key;
            ev(OpType::Compare, {}, {tag(node.first, Role::Compared)}, "Compare " + to_string(node.second) + " with " + to_string(key) + (eq ? ": found." : ": different."), 6);
            if (eq) {
                if (!remove) { clearProbe("Found " + to_string(key) + " in chain " + to_string(h) + ".", {tag(node.first, Role::Found)}); return; }
                vector<Delta> d;
                if (i + 1 < chain.size()) d.push_back(Delta::edgeAdd(prev, chain[i + 1].first, Slot::Next, 1, 0));   // bypass
                else d.push_back(Delta::edgeRemove(prev, node.first, Slot::Next, 0));
                d.push_back(Delta::destroy(node.first, 0));
                d.push_back(Delta::pointer("probe", -1, 0));
                chain.erase(chain.begin() + i);
                ev(OpType::Delete, d, {}, "Unlink " + to_string(key) + " from chain " + to_string(h) + " and free its node.", 6);
                return;
            }
            prev = node.first;
        }
        clearProbe(to_string(key) + " is not in chain " + to_string(h) + ".", {tag(h, Role::NotFound)});
    }

    HashMethod method_;
    AlgorithmInfo info_;
    Recorder* rec_ = nullptr;
    int m_ = 7;
    vector<int> keys_;
    vector<int> state_;
    vector<vector<pair<int, int>>> chains_;   // (node id, key)
    int nextChain_ = CHAIN_BASE;
};

inline void registerHashing(AlgorithmCatalog& cat) {
    for (HashMethod m : {HashMethod::Linear, HashMethod::Quadratic, HashMethod::Chaining}) {
        CatalogEntry e;
        HashTableAlgorithm probe(m);
        e.id = probe.info().id; e.category = "Hashing"; e.name = probe.info().name;
        e.input = InputKind::Structure; e.structure = StructureKind::Hash; e.ops = HashTableAlgorithm::ops();
        e.make = [m] { return unique_ptr<Algorithm>(new HashTableAlgorithm(m)); };
        cat.add(e);
    }
}

#endif
