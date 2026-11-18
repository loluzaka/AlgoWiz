#ifndef ALGORITHMLISTPANEL_H
#define ALGORITHMLISTPANEL_H

// Left-hand ALGORITHMS column (the QTreeWidget of the Qt plan): categories with their
// algorithms, the one being visualised is marked with ">".

#include <string>

#include "../../core/AlgorithmCatalog.h"
#include "../../render/primitives/Canvas.h"
#include "../../render/theme/Theme.h"

using namespace std;

inline void drawAlgorithmList(Canvas& c, int x, int y, int w, int h, const AlgorithmCatalog& cat, const vector<string>& categories, const string& currentId, const Theme& th) {
    c.put(x, y, "ALGORITHMS", th.header);
    vector<pair<string, Style>> rows;
    int activeRow = 0;
    for (const string& category : categories) {
        auto entries = cat.inCategory(category);
        if (entries.empty()) continue;
        rows.push_back({category, th.accent});
        for (auto* e : entries) {
            bool cur = e->id == currentId;
            if (cur) activeRow = (int)rows.size();
            Style st = cur ? th.header : th.normal;
            rows.push_back({string(cur ? "> " : "  ") + e->name, st});
        }
    }
    int visible = h - 1;
    int first = max(0, min(activeRow - visible / 2, (int)rows.size() - visible));
    for (int i = 0; i < visible && first + i < (int)rows.size(); i++) {
        string t = rows[first + i].first;
        auto ch = utf8Chars(t);
        if ((int)ch.size() > w - 1) { t.clear(); for (int k = 0; k < w - 1; k++) t += ch[k]; }
        c.put(x, y + 1 + i, t, rows[first + i].second);
    }
}

#endif
