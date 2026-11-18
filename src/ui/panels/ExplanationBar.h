#ifndef EXPLANATIONBAR_H
#define EXPLANATIONBAR_H

// "CURRENT OPERATION" bar (plain-language explanation of the step) and the key-help line at the
// bottom of the screen, plus the shared word-wrap helper.

#include <string>
#include <vector>

#include "../../engine/Describe.h"
#include "../../render/primitives/Canvas.h"
#include "../../render/theme/Theme.h"

using namespace std;

inline vector<string> wrapText(const string& text, int width) {
    vector<string> lines;
    if (width < 4) return lines;
    string cur;
    size_t i = 0;
    while (i < text.size()) {
        size_t j = text.find(' ', i);
        string word = text.substr(i, j == string::npos ? string::npos : j - i);
        i = j == string::npos ? text.size() : j + 1;
        while (textWidth(word) > width) {                 // a single word longer than the line
            if (!cur.empty()) { lines.push_back(cur); cur.clear(); }
            lines.push_back(word.substr(0, width));
            word = word.substr(width);
        }
        if (cur.empty()) cur = word;
        else if (textWidth(cur) + 1 + textWidth(word) <= width) cur += " " + word;
        else { lines.push_back(cur); cur = word; }
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

inline void drawExplanationBar(Canvas& c, int x, int y, int w, int h, const Step* step, size_t position, size_t total, const Theme& th) {
    string head = "CURRENT OPERATION";
    if (step) head += "  " + describeStep(*step);
    c.put(x, y, head, th.header);
    string text;
    if (!step) text = total == 0 ? "Nothing to show: the algorithm produced no steps for this input." : "Ready. Press N or the Right arrow for the first step, or P to play.";
    else text = step->note.empty() ? "(no explanation for this step)" : step->note;
    if (step && position == total && total > 0) text += "  [end of execution]";
    vector<string> lines = wrapText(text, w);
    for (int i = 0; i < h - 1 && i < (int)lines.size(); i++) c.put(x, y + 1 + i, lines[i], th.normal);
}

// Key help. Two lines when the terminal is wide, wraps otherwise.
inline void drawControls(Canvas& c, int x, int y, int w, int h, bool playing, const Theme& th) {
    string line = string("[P] ") + (playing ? "Pause" : "Play") + "  [Space] Pause/Resume  [N/Right] Next  [B/Left] Previous  [R] Reset  [S] Speed  "
                  "[I] Input  [C] Code  [Q] Back";
    vector<string> lines = wrapText(line, w);
    for (int i = 0; i < h && i < (int)lines.size(); i++) c.put(x, y + i, lines[i], th.dim);
}

#endif
