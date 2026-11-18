#ifndef PLUGINCONTEXT_H
#define PLUGINCONTEXT_H

// The object a plugin writes its steps into. It is a thin, safer facade over Recorder /
// ArrayTrace: the plugin never sees the Timeline, the engine or any renderer.
//   Easy path : setArray(), compare(), swap(), assign(), mark(), highlight(), pointer(), note()
//   Raw path  : emit() with hand-made deltas (checked afterwards by StepValidator)

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../engine/Recorder.h"

using namespace std;

class PluginContext {
public:
    explicit PluginContext(Recorder& r) : rec_(r) {}

    // ---- array helpers (container 0) ----
    void setArray(const vector<int>& v) { trace_.reset(new ArrayTrace<int>(rec_, v, 0)); }
    int size() const { return trace_ ? trace_->size() : 0; }
    int get(int i) const { return (*trace_)[i]; }
    void compare(int i, int j, const string& note, int line = -1) { need().compare(i, j, note, line); }
    void swap(int i, int j, const string& note, int line = -1) { need().swap(i, j, note, line); }
    void assign(int i, int v, const string& note, int line = -1) { need().set(i, v, note, line); }
    void mark(const vector<int>& ids, Role r, const string& note, int line = -1) { need().mark(ids, r, note, line); }
    void highlight(const vector<int>& ids, Role r, const string& note, int line = -1) { need().highlight(ids, r, note, line); }
    void pointer(const string& label, int idx, const string& note, int line = -1) { need().pointer(label, idx, note, line); }
    void note(const string& text, int line = -1) { rec_.note(text, line); }

    // ---- raw access (custom structures, anything the helpers do not cover) ----
    void setup(const vector<Delta>& deltas) { rec_.setup(deltas); }
    void emit(OpType op, vector<Delta> deltas, vector<RoleTag> roles, const string& note, int line = -1) {
        rec_.emit(op, std::move(deltas), std::move(roles), note, line);
    }
    Recorder& recorder() { return rec_; }

private:
    ArrayTrace<int>& need() {
        if (!trace_) throw runtime_error("Plugin error: call setArray() before using array helpers.");
        return *trace_;
    }
    Recorder& rec_;
    unique_ptr<ArrayTrace<int>> trace_;
};

#endif
