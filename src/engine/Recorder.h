#ifndef RECORDER_H
#define RECORDER_H

#include <string>
#include <utility>
#include <vector>

#include "Step.h"

using namespace std;

// Helper every algorithm uses to write Steps into a Timeline.
// While `muted` is true (silent replay of earlier commands) emitted deltas are
// folded into Timeline::initial instead of becoming visible steps.
class Recorder {
public:
    explicit Recorder(Timeline& t) : tl(t) {}
    bool muted = false;
    string operation;

    void setup(const vector<Delta>& ds) { for (const Delta& d : ds) tl.initial.push_back(d); }

    void emit(OpType op, vector<Delta> deltas, vector<RoleTag> roles, const string& note,
              int codeLine = -1, float progress = -1.f) {
        if (muted) { for (const Delta& d : deltas) tl.initial.push_back(d); return; }
        Step s;
        s.op = op; s.operation = operation; s.deltas = std::move(deltas); s.roles = std::move(roles);
        s.note = note; s.codeLine = codeLine; s.progress = progress;
        tl.steps.push_back(std::move(s));
    }
    void note(const string& text, int codeLine = -1) { emit(OpType::Note, {}, {}, text, codeLine); }
    Timeline& timeline() { return tl; }
private:
    Timeline& tl;
};

inline Delta valueDelta(int id, int v, int c) { return Delta::value(id, v, c); }
inline Delta valueDelta(int id, const string& v, int c) { return Delta::valueText(id, v, c); }
inline string show(int v) { return to_string(v); }
inline string show(const string& s) { return "\"" + s + "\""; }

// An array whose every change is recorded. The algorithm mutates data ONLY through
// these methods, so the recorded steps can never disagree with what really happened.
template <class T>
class ArrayTrace {
public:
    // visible == false creates an auxiliary row with `init.size()` empty slots (nodes appear later).
    ArrayTrace(Recorder& r, vector<T> init, int container = 0, bool visible = true) : rec(r), a(std::move(init)), c(container) {
        vector<Delta> ds;
        ds.push_back(Delta::resize((int)a.size(), c));
        if (visible)
            for (size_t i = 0; i < a.size(); i++) ds.push_back(valueDelta((int)i, a[i], c));
        rec.setup(ds);
    }
    // Copies src[l..r] into the shadow vector WITHOUT emitting a step (the caller emits one itself).
    void loadRange(int l, int r, const vector<T>& src) { for (int i = l; i <= r; i++) a[i] = src[i]; }
    int size() const { return (int)a.size(); }
    const T& operator[](size_t i) const { return a[i]; }
    const vector<T>& data() const { return a; }

    void compare(int i, int j, const string& note, int line = -1) {
        rec.emit(OpType::Compare, {}, {tag(i, Role::Compared), tag(j, Role::Compared)}, note, line);
    }
    void compareValue(int i, const string& note, int line = -1) {      // element vs. a held key / pivot
        rec.emit(OpType::Compare, {}, {tag(i, Role::Compared)}, note, line);
    }
    void swap(int i, int j, const string& note, int line = -1) {
        std::swap(a[i], a[j]);
        rec.emit(OpType::Swap, {valueDelta(i, a[i], c), valueDelta(j, a[j], c)},
                 {tag(i, Role::Moved), tag(j, Role::Moved)}, note, line);
    }
    void set(int i, const T& v, const string& note, int line = -1) {
        a[i] = v;
        rec.emit(OpType::Assign, {valueDelta(i, v, c)}, {tag(i, Role::Moved)}, note, line);
    }
    void mark(const vector<int>& ids, Role r, const string& note, int line = -1) {
        vector<Delta> ds;
        for (int i : ids) ds.push_back(Delta::mark(i, r, c));
        rec.emit(OpType::Mark, ds, {}, note, line);
    }
    void unmark(const vector<int>& ids, const string& note, int line = -1) {
        vector<Delta> ds;
        for (int i : ids) ds.push_back(Delta::mark(i, Role::Default, c));
        rec.emit(OpType::Unmark, ds, {}, note, line);
    }
    void highlight(const vector<int>& ids, Role r, const string& note, int line = -1) {
        vector<RoleTag> t;
        for (int i : ids) t.push_back(tag(i, r));
        rec.emit(OpType::Highlight, {}, t, note, line);
    }
    void pointer(const string& label, int idx, const string& note, int line = -1) {
        rec.emit(OpType::PointerMove, {Delta::pointer(label, idx, c)},
                 idx >= 0 ? vector<RoleTag>{tag(idx, Role::Current)} : vector<RoleTag>{}, note, line);
    }
    void iterate(const string& note, int line = -1) { rec.emit(OpType::Iterate, {}, {}, note, line); }
    RoleTag tag(int i, Role r) const { RoleTag t; t.target_id = i; t.role = r; t.container = c; return t; }

    Recorder& rec;
private:
    vector<T> a;
    int c;
};

#endif
