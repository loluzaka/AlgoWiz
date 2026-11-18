#ifndef CODEPANEL_H
#define CODEPANEL_H

// Shows operation pseudocode separately from the C++ source block mapped to the current step.

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include "../../engine/Algorithm.h"
#include "../../engine/ExecutionEngine.h"
#include "../../render/primitives/Canvas.h"
#include "../../render/theme/Theme.h"

using namespace std;

inline set<int> executedLines(const Timeline& tl, size_t position) {
    set<int> s;
    for (size_t i = 0; i < position && i < tl.steps.size(); i++)
        if (tl.steps[i].codeLine >= 0) s.insert(tl.steps[i].codeLine);
    return s;
}

inline string codeLineText(const string& text, int width) {
    if (textWidth(text) <= width) return text;
    string result;
    for (const string& ch : utf8Chars(text)) {
        if (textWidth(result) + textWidth(ch) >= width) break;
        result += ch;
    }
    return result;
}

inline void drawCodePanel(Canvas& c, int x, int y, int w, int h, const AlgorithmInfo& info, const ExecutionEngine& e, const Theme& th) {
    if (h <= 0 || w <= 0) return;

    const Step* current = e.currentStep();
    const Step* displayed = current ? current : e.nextStep();
    string operation = displayed ? displayed->operation : "";
    const CppSourceBlock* source = nullptr;
    if (displayed) {
        auto lineIt = info.cppSourceBlocks.find(displayed->codeLine);
        if (lineIt != info.cppSourceBlocks.end()) {
            auto sourceIt = lineIt->second.find(displayed->op);
            if (sourceIt != lineIt->second.end()) source = &sourceIt->second;
        }
    }

    vector<int> lines;
    auto specific = info.operationPseudocodeLines.find(operation);
    if (specific != info.operationPseudocodeLines.end()) lines = specific->second;
    else for (int i = 0; i < (int)info.pseudocode.size(); i++) lines.push_back(i);

    string operationTitle = operation.empty() ? "ALGORITHM PSEUDOCODE" : "OPERATION PSEUDOCODE: " + operation;
    for (char& ch : operationTitle) ch = (char)toupper((unsigned char)ch);

    int sourceHeight = source ? min(h - 2, (int)source->lines.size() + 1) : min(2, h);
    int operationHeight = max(1, h - sourceHeight);
    c.put(x, y, operationTitle, th.header);
    set<int> done = executedLines(e.timeline(), e.position());
    int active = current ? current->codeLine : -1;
    for (int i = 0; i < (int)lines.size() && i + 1 < operationHeight; i++) {
        int line = lines[i];
        if (line < 0 || line >= (int)info.pseudocode.size()) continue;
        string text = codeLineText(info.pseudocode[line], max(0, w - 1));
        Style style = line == active ? th.header : done.count(line) ? th.dim : th.normal;
        if (line == active) style.inverse = style.bold = true;
        if (line == active) text += string(max(0, w - 1 - textWidth(text)), ' ');
        c.put(x, y + 1 + i, text, style);
    }
    if (lines.empty() && operationHeight > 1)
        c.put(x, y + 1, "(no pseudocode available)", th.dim);

    int currentY = y + operationHeight;
    if (currentY < y + h) {
        string title = source ? "C++ SOURCE: " + source->location : "C++ SOURCE";
        c.put(x, currentY, title, th.header);
    }
    if (source) {
        for (int i = 0; i < (int)source->lines.size() && currentY + 1 + i < y + h; i++) {
            string text = codeLineText(source->lines[i], max(0, w - 1));
            Style style = i == source->activeLine ? th.header : th.normal;
            if (i == source->activeLine) {
                style.inverse = style.bold = true;
                text += string(max(0, w - 1 - textWidth(text)), ' ');
            }
            c.put(x, currentY + 1 + i, text, style);
        }
    } else if (currentY + 1 < y + h) {
        string text = displayed ? "(C++ source block unavailable)" : "(advance to see the current step)";
        c.put(x, currentY + 1, codeLineText(text, max(0, w - 1)), th.dim);
    }
}

#endif
