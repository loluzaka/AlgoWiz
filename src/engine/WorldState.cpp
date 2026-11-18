#include "WorldState.h"

#include <algorithm>
#include <sstream>

Container& WorldState::get(int container) { return containers[container]; }

const Container* WorldState::find(int container) const {
    auto it = containers.find(container);
    return it == containers.end() ? nullptr : &it->second;
}

Role WorldState::markOf(int container, int id) const {
    const Container* c = find(container);
    if (!c) return Role::Default;
    auto it = c->marks.find(id);
    return it == c->marks.end() ? Role::Default : it->second;
}

static void setValue(NodeState& n, const Delta& d) {
    n.value = d.new_value;
    if (!d.text.empty()) { n.text = d.text; n.hasText = true; }
    else { n.text.clear(); n.hasText = false; }
}

void WorldState::apply(const Delta& d) {
    Container& c = containers[d.container];
    switch (d.kind) {
    case Delta::Kind::ValueChange:
    case Delta::Kind::NodeCreate:
        setValue(c.nodes[d.target_id], d);          // ValueChange auto-creates the node
        break;
    case Delta::Kind::NodeDelete:
        c.nodes.erase(d.target_id);
        c.edges.erase(d.target_id);
        c.marks.erase(d.target_id);
        for (auto it = c.edges.begin(); it != c.edges.end();) {      // drop edges that would dangle
            auto& v = it->second;
            v.erase(remove_if(v.begin(), v.end(),
                              [&](const EdgeState& e) { return e.to == d.target_id; }), v.end());
            it = v.empty() ? c.edges.erase(it) : std::next(it);
        }
        break;
    case Delta::Kind::EdgeAdd: {
        auto& v = c.edges[d.target_id];
        EdgeState e; e.to = d.edge_to; e.slot = d.slot; e.weight = d.weight;
        bool placed = false;
        for (auto& old : v) {
            bool sameSlot = (d.slot != Slot::None && old.slot == d.slot);   // pointer semantics: replace
            bool sameDest = (d.slot == Slot::None && old.slot == Slot::None && old.to == d.edge_to);
            if (sameSlot || sameDest) { old = e; placed = true; break; }
        }
        if (!placed) v.push_back(e);
        break;
    }
    case Delta::Kind::EdgeRemove: {
        auto it = c.edges.find(d.target_id);
        if (it == c.edges.end()) break;
        auto& v = it->second;
        v.erase(remove_if(v.begin(), v.end(), [&](const EdgeState& e) {
            if (d.edge_to >= 0) return e.to == d.edge_to && (d.slot == Slot::None || e.slot == d.slot);
            return d.slot != Slot::None && e.slot == d.slot;
        }), v.end());
        if (v.empty()) c.edges.erase(it);
        break;
    }
    case Delta::Kind::RoleChange: {
        Role r = static_cast<Role>(d.new_value);
        if (r == Role::Default) c.marks.erase(d.target_id);
        else c.marks[d.target_id] = r;
        break;
    }
    case Delta::Kind::Resize:
        c.capacity = d.new_value;
        for (auto it = c.nodes.begin(); it != c.nodes.end();)
            it = (it->first >= d.new_value) ? c.nodes.erase(it) : std::next(it);
        break;
    case Delta::Kind::PointerSet:
        if (d.target_id < 0) c.pointers.erase(d.text);
        else c.pointers[d.text] = d.target_id;
        break;
    case Delta::Kind::Label:
        c.nodes[d.target_id].label = d.text;
        break;
    case Delta::Kind::Clear:
        c = Container();
        break;
    }
}

void WorldState::apply(const vector<Delta>& ds) { for (const Delta& d : ds) apply(d); }

string WorldState::signature() const {
    ostringstream o;
    for (const auto& cp : containers) {
        const Container& c = cp.second;
        o << "C" << cp.first << "[cap=" << c.capacity << "]";
        for (const auto& n : c.nodes) {
            o << " n" << n.first << "=";
            if (n.second.hasText) o << '"' << n.second.text << '"'; else o << n.second.value;
            if (!n.second.label.empty()) o << "{" << n.second.label << "}";
        }
        for (const auto& e : c.edges)
            for (const auto& x : e.second) o << " e" << e.first << ">" << x.to << "/" << (int)x.slot << "/" << x.weight;
        for (const auto& p : c.pointers) o << " p:" << p.first << "@" << p.second;
        for (const auto& m : c.marks) o << " m" << m.first << ":" << (int)m.second;
        o << "\n";
    }
    return o.str();
}
