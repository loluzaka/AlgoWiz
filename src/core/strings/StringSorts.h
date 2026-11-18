#ifndef STRINGSORTS_H
#define STRINGSORTS_H

// String / alphabetical sorting. The comparison logic lives here, completely separate
// from numeric comparison (spec section 11); the sorting loops are the shared templates
// from ComparisonSorts.h.

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "../sorting/ComparisonSorts.h"

using namespace std;

inline string foldCase(string s) {
    for (char& ch : s) ch = (char)tolower((unsigned char)ch);
    return s;
}

inline string stringModeName(StringMode m, bool ci) {
    string base;
    switch (m) {
    case StringMode::Lexicographic:        base = "alphabetical ascending"; break;
    case StringMode::LexicographicReverse: base = "alphabetical descending (reverse)"; break;
    case StringMode::Length:               base = "length ascending"; break;
    case StringMode::LengthReverse:        base = "length descending"; break;
    }
    if (m == StringMode::Lexicographic || m == StringMode::LexicographicReverse)
        base += ci ? ", case-insensitive" : ", case-sensitive";
    return base;
}

// Character-by-character explanation: "a = a, p = p, p < r -> \"apple\" sorts before \"apricot\"".
inline string characterComparison(const string& x, const string& y, bool ci) {
    string a = ci ? foldCase(x) : x, b = ci ? foldCase(y) : y;
    string out;
    size_t n = min(a.size(), b.size());
    for (size_t i = 0; i < n; i++) {
        if (i) out += ", ";
        if (a[i] == b[i]) { out += string(1, a[i]) + " = " + string(1, b[i]); continue; }
        out += string(1, a[i]) + (a[i] < b[i] ? " < " : " > ") + string(1, b[i]);
        out += a[i] < b[i] ? ": \"" + x + "\" sorts before \"" + y + "\"" : ": \"" + y + "\" sorts before \"" + x + "\"";
        return out;
    }
    if (a.size() == b.size()) return out + " (identical)";
    return out + (a.size() < b.size() ? ": \"" + x + "\" is a prefix, so it is shorter and comes first"
                                      : ": \"" + y + "\" is a prefix, so it is shorter and comes first");
}

inline SortCmp<string> makeStringCmp(StringMode mode, bool ci) {
    SortCmp<string> c;
    switch (mode) {
    case StringMode::Lexicographic:
        c.before = [ci](const string& x, const string& y) { return ci ? foldCase(x) < foldCase(y) : x < y; };
        c.detail = [ci](const string& x, const string& y) { return characterComparison(x, y, ci); };
        break;
    case StringMode::LexicographicReverse:
        c.before = [ci](const string& x, const string& y) { return ci ? foldCase(x) > foldCase(y) : x > y; };
        c.detail = [ci](const string& x, const string& y) { return characterComparison(x, y, ci); };
        break;
    case StringMode::Length:
        c.before = [](const string& x, const string& y) { return x.size() < y.size(); };
        c.detail = [](const string& x, const string& y) { return "length " + to_string(x.size()) + " vs " + to_string(y.size()); };
        break;
    case StringMode::LengthReverse:
        c.before = [](const string& x, const string& y) { return x.size() > y.size(); };
        c.detail = [](const string& x, const string& y) { return "length " + to_string(x.size()) + " vs " + to_string(y.size()); };
        break;
    }
    return c;
}

class StringSort : public Algorithm {
public:
    explicit StringSort(CmpSortKind k) : kind_(k), info_(comparisonSortInfo(k)) {
        info_.id = "str_" + info_.id;
        info_.name = info_.name + " (strings)";
        info_.description += " Here it orders words using the selected string criterion.";
    }
    const AlgorithmInfo& info() const override { return info_; }

    string validate(const Input& in) const override {
        if (in.words.size() > 14) return "Please enter at most 14 words so they fit on screen.";
        for (const string& w : in.words) {
            if (w.empty()) return "Empty words are not allowed.";
            if (w.size() > 14) return "The word \"" + w + "\" is longer than 14 characters.";
        }
        return "";
    }

    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = info_.id;
        tl.structure = StructureKind::Array;
        tl.containerNames = {"Words", "Temporary"};
        tl.inputDescription = to_string(in.words.size()) + " words, " + stringModeName(in.stringMode, in.caseInsensitive);
        Recorder rec(tl);
        SortCmp<string> cmp = makeStringCmp(in.stringMode, in.caseInsensitive);
        ArrayTrace<string> t(rec, in.words, 0);
        rec.note("Sorting criterion: " + stringModeName(in.stringMode, in.caseInsensitive) + ".", 0);
        switch (kind_) {
        case CmpSortKind::Bubble:    bubbleSort(t, cmp); break;
        case CmpSortKind::Selection: selectionSort(t, cmp); break;
        case CmpSortKind::Insertion: insertionSort(t, cmp); break;
        case CmpSortKind::Quick:     quickSort(t, cmp); break;
        case CmpSortKind::Merge: {
            ArrayTrace<string> aux(rec, in.words, 1, false);
            mergeSort(t, aux, cmp);
            break;
        }
        }
        return tl;
    }
private:
    CmpSortKind kind_;
    AlgorithmInfo info_;
};

inline void registerStrings(AlgorithmCatalog& cat) {
    for (CmpSortKind k : {CmpSortKind::Bubble, CmpSortKind::Selection, CmpSortKind::Insertion, CmpSortKind::Merge, CmpSortKind::Quick}) {
        CatalogEntry e;
        StringSort probe(k);
        e.id = probe.info().id; e.category = "Sorting"; e.group = "String sorting"; e.name = probe.info().name;
        e.input = InputKind::Words; e.structure = StructureKind::Array;
        e.make = [k] { return unique_ptr<Algorithm>(new StringSort(k)); };
        cat.add(e);
    }
}

#endif
