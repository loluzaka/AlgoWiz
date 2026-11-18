#ifndef INFOPANEL_H
#define INFOPANEL_H

// Right-hand INFORMATION column: theoretical complexity and counters calculated through
// the current recorded step.

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../../engine/Algorithm.h"
#include "../../engine/ExecutionEngine.h"
#include "../../render/primitives/Canvas.h"
#include "../../render/theme/Theme.h"

using namespace std;

inline string progressBar(double fraction, int width) {
    int fill = (int)(fraction * width + 0.5);
    return "[" + string(max(0, min(width, fill)), '#') + string(max(0, width - fill), '-') + "]";
}

inline string recordedTime(double milliseconds) {
    char buf[48];
    snprintf(buf, sizeof buf, "%.2f ms", milliseconds);
    return buf;
}

// compact == true keeps both complexity sections and core live counters visible with code.
inline void drawInfoPanel(Canvas& c, int x, int y, int w, int h, const AlgorithmInfo& info, const ExecutionEngine& e, const Theme& th, bool compact) {
    vector<pair<string, Style>> lines;
    lines.push_back({"INFORMATION", th.header});
    lines.push_back({info.name, th.accent});
    lines.push_back({"TIME COMPLEXITY", th.header});
    lines.push_back({"Best:    " + info.best, th.normal});
    lines.push_back({"Average: " + info.average, th.normal});
    lines.push_back({"Worst:   " + info.worst, th.normal});
    lines.push_back({"SPACE COMPLEXITY", th.header});
    lines.push_back({"Space:   " + info.space, th.normal});
    lines.push_back({"LIVE EXECUTION", th.header});
    lines.push_back({"Steps: " + to_string(e.position()) + " / " + to_string(e.total()), th.normal});
    if (compact) {
        lines.push_back({"Cmp: " + to_string(e.stats().comparisons) + " Swp: " + to_string(e.stats().swaps), th.normal});
        lines.push_back({"Asg: " + to_string(e.stats().assignments) + " Acc: " + to_string(e.stats().accesses), th.normal});
        lines.push_back({"Recorded: " + recordedTime(e.timeline().runtimeMs), th.dim});
    } else {
        for (const auto& r : e.stats().rows()) {
            string label = r.first + ":";
            string value = to_string(r.second);
            int pad = max(1, w - textWidth(label) - textWidth(value));
            lines.push_back({label + string(pad, ' ') + value, th.normal});
        }
        lines.push_back({"Recorded: " + recordedTime(e.timeline().runtimeMs), th.dim});
        lines.push_back({"", th.normal});
        lines.push_back({"PROGRESS", th.header});
        lines.push_back({progressBar(e.total() ? (double)e.position() / e.total() : 0.0, max(4, w - 8)) + " " + to_string(e.total() ? (int)(100.0 * e.position() / e.total()) : 0) + "%", th.dim});
    }

    for (int i = 0; i < h && i < (int)lines.size(); i++) {
        string t = lines[i].first;
        if (textWidth(t) > w) { auto ch = utf8Chars(t); t.clear(); for (int k = 0; k < w; k++) t += ch[k]; }
        c.put(x, y + i, t, lines[i].second);
    }
}

#endif
