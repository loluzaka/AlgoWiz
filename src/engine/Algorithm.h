#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <chrono>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "Input.h"
#include "Step.h"

using namespace std;

struct CppSourceBlock {
    string location;
    vector<string> lines;
    int activeLine = -1;
};

// Static facts shown in the info panel, the code panel and the exported report.
struct AlgorithmInfo {
    string id;            // stable key, e.g. "bubble_sort"
    string name;          // display name, e.g. "Bubble Sort"
    string category;      // "Sorting", "Searching", ...
    string best, average, worst, space;
    string description;
    vector<string> pseudocode;   // Step::codeLine indexes into this
    map<string, vector<int>> operationPseudocodeLines;
    map<int, map<OpType, CppSourceBlock>> cppSourceBlocks; // keyed by pseudocode line, then step type
};

inline void addCppSourceBlock(AlgorithmInfo& info, int codeLine, OpType op, const string& location,
                              const vector<string>& lines, int activeLine = 1) {
    info.cppSourceBlocks[codeLine][op] = {location, lines, activeLine};
}

// Contract: every built-in and plugin algorithm implements this interface.
// run() RECORDS a Timeline; it never draws and never talks to the UI.
class Algorithm {
public:
    virtual ~Algorithm() = default;
    virtual const AlgorithmInfo& info() const = 0;
    // Return an empty string when the input is acceptable, otherwise an understandable message.
    virtual string validate(const Input&) const { return ""; }
    virtual Timeline run(const Input&) = 0;
};

// Thrown by data-structure operations for normal invalid user actions (overflow, bad position ...).
// generate() turns it into Timeline::error with the plain message, so the UI never crashes.
struct InvalidOperation : runtime_error {
    explicit InvalidOperation(const string& m) : runtime_error(m) {}
};

// Fills seq, resolves progress == -1 and makes progress monotonic in [0,1].
inline void finalizeTimeline(Timeline& tl) {
    size_t n = tl.steps.size();
    float prev = 0.f;
    for (size_t i = 0; i < n; i++) {
        Step& s = tl.steps[i];
        s.seq = i;
        float p = s.progress < 0.f ? (n <= 1 ? 1.f : (float)(i + 1) / (float)n) : s.progress;
        if (p > 1.f) p = 1.f;
        if (p < prev) p = prev;
        s.progress = prev = p;
    }
    if (n > 0) tl.steps.back().progress = 1.f;
}

// The single entry point used by the UI, Race, Quiz, Report and the tests.
// It validates the input, times the run and converts any exception into Timeline::error.
inline Timeline generate(Algorithm& algo, const Input& in) {
    Timeline tl;
    tl.algorithm = algo.info().id;
    string problem = algo.validate(in);
    if (!problem.empty()) { tl.error = problem; return tl; }
    auto t0 = chrono::steady_clock::now();
    try {
        tl = algo.run(in);
    } catch (const InvalidOperation& e) {
        tl = Timeline(); tl.algorithm = algo.info().id;
        tl.error = e.what();
        return tl;
    } catch (const exception& e) {
        tl = Timeline(); tl.algorithm = algo.info().id;
        tl.error = string("Algorithm failed: ") + e.what();
        return tl;
    } catch (...) {
        tl = Timeline(); tl.algorithm = algo.info().id;
        tl.error = "Algorithm failed with an unknown error.";
        return tl;
    }
    auto t1 = chrono::steady_clock::now();
    tl.runtimeMs = chrono::duration<double, milli>(t1 - t0).count();
    if (tl.algorithm.empty()) tl.algorithm = algo.info().id;
    finalizeTimeline(tl);
    return tl;
}

#endif
