#ifndef STRUCTUREALGORITHM_H
#define STRUCTUREALGORITHM_H

#include <algorithm>
#include <string>
#include <vector>

#include "Algorithm.h"
#include "Recorder.h"

using namespace std;

// Base class for every data structure that is driven by a list of Commands
// (linked lists, stack, queue, hash table, trees, heaps, graphs, custom structures).
//
// run() works like a session replay:
//   1. begin()            - the subclass resets its model and declares containers
//   2. initial values     - built SILENTLY (muted recorder: deltas go to Timeline::initial)
//   3. commands before Input::visibleFrom run silently as well
//   4. the remaining commands are recorded as visible steps
// This is what lets the UI keep a history of operations while every Timeline starts at
// the state the user already saw.
class StructureAlgorithm : public Algorithm {
public:
    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = info().id;
        tl.structure = structureKind();
        tl.containerNames = containerNames();
        Recorder rec(tl);
        rec.muted = true;
        begin(in, rec);
        for (int v : in.values) addInitial(v, rec);
        size_t split = min(in.visibleFrom, in.commands.size());
        for (size_t i = 0; i < split; i++) {
            rec.operation = in.commands[i].op;
            execute(in.commands[i], rec);
            rec.operation.clear();
            cleanup(rec);
        }
        rec.muted = false;
        for (size_t i = split; i < in.commands.size(); i++) {
            if (i > split) { rec.operation.clear(); cleanup(rec); }
            rec.operation = in.commands[i].op;
            execute(in.commands[i], rec);
            rec.operation.clear();
        }
        tl.inputDescription = describe(in);
        return tl;
    }

protected:
    virtual StructureKind structureKind() const = 0;
    virtual vector<string> containerNames() const { return {"Structure"}; }
    virtual void begin(const Input& in, Recorder& rec) = 0;
    virtual void addInitial(int value, Recorder& rec) = 0;
    virtual void execute(const Command& cmd, Recorder& rec) = 0;
    virtual void cleanup(Recorder&) {}     // remove persistent highlight marks left by the previous command
    virtual string describe(const Input& in) const {
        return to_string(in.values.size()) + " initial value(s), " + to_string(in.commands.size()) + " operation(s)";
    }
};

#endif
