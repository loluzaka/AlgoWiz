#ifndef SEARCHES_H
#define SEARCHES_H

// Linear, Binary, Interpolation and Median search, plus registerSearching().
//
// DECISION (Median Search): the specification leaves the definition open. We implement
// "Median Search" as Quickselect: it finds the (lower) median of an UNSORTED array by
// repeatedly partitioning and keeping only the side that contains the median position.
// If the instructor defines it differently only this class has to change.

#include <algorithm>
#include <string>
#include <vector>

#include "../../engine/Algorithm.h"
#include "../../engine/Recorder.h"
#include "../AlgorithmCatalog.h"

using namespace std;

enum class SearchKind { Linear, Binary, Interpolation, Median };

inline AlgorithmInfo searchInfo(SearchKind k) {
    AlgorithmInfo a;
    a.category = "Searching";
    switch (k) {
    case SearchKind::Linear:
        a.id = "linear_search"; a.name = "Linear Search";
        a.best = "O(1)"; a.average = "O(n)"; a.worst = "O(n)"; a.space = "O(1)";
        a.description = "Checks every element from left to right until the target is found. Works on unsorted data.";
        a.pseudocode = {"for i = 0 .. n-1:", "  if a[i] == target:", "    return i", "return NOT FOUND"};
        break;
    case SearchKind::Binary:
        a.id = "binary_search"; a.name = "Binary Search";
        a.best = "O(1)"; a.average = "O(log n)"; a.worst = "O(log n)"; a.space = "O(1)";
        a.description = "Needs sorted data. Compares the target with the middle element and discards half of the range each time.";
        a.pseudocode = {"lo = 0; hi = n-1", "while lo <= hi:", "  mid = lo + (hi-lo)/2", "  if a[mid] == target: return mid", "  if a[mid] < target: lo = mid+1", "  else: hi = mid-1", "return NOT FOUND"};
        break;
    case SearchKind::Interpolation:
        a.id = "interpolation_search"; a.name = "Interpolation Search";
        a.best = "O(1)"; a.average = "O(log log n)"; a.worst = "O(n)"; a.space = "O(1)";
        a.description = "Needs sorted numeric data. Estimates the position from the target's value, like looking up a name in a phone book.";
        a.pseudocode = {"lo = 0; hi = n-1", "while lo <= hi and a[lo] <= target <= a[hi]:", "  pos = lo + (target-a[lo]) * (hi-lo) / (a[hi]-a[lo])",
                        "  if a[pos] == target: return pos", "  if a[pos] < target: lo = pos+1", "  else: hi = pos-1", "return NOT FOUND"};
        break;
    case SearchKind::Median:
        a.id = "median_search"; a.name = "Median Search (Quickselect)";
        a.best = "O(n)"; a.average = "O(n)"; a.worst = "O(n^2)"; a.space = "O(1)";
        a.description = "Finds the median of an unsorted array without sorting it: partition, then keep only the side holding the median position.";
        a.pseudocode = {"k = (n-1)/2; lo = 0; hi = n-1", "while lo <= hi:", "  p = partition(lo, hi)   (pivot = a[hi])", "  if p == k: return a[p]", "  if p < k: lo = p+1", "  else: hi = p-1"};
        break;
    }
    return a;
}

class SearchAlgorithm : public Algorithm {
public:
    explicit SearchAlgorithm(SearchKind k) : kind_(k), info_(searchInfo(k)) {}
    const AlgorithmInfo& info() const override { return info_; }

    string validate(const Input& in) const override {
        if (in.values.empty()) return "The array is empty. Enter at least one value before searching.";
        if (kind_ != SearchKind::Median && !in.hasTarget) return "Please choose a search target.";
        if (kind_ == SearchKind::Binary || kind_ == SearchKind::Interpolation) {
            if (!is_sorted(in.values.begin(), in.values.end()))
                return string(kind_ == SearchKind::Binary ? "Binary" : "Interpolation") +
                       " search requires data sorted in ascending order. Sort the array first or use a preset.";
        }
        return "";
    }

    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = info_.id;
        tl.structure = StructureKind::Array;
        tl.containerNames = {"Array"};
        tl.inputDescription = to_string(in.values.size()) + " values" + (kind_ == SearchKind::Median ? "" : ", target " + to_string(in.target));
        Recorder rec(tl);
        ArrayTrace<int> t(rec, in.values, 0);
        switch (kind_) {
        case SearchKind::Linear: linear(t, in.target); break;
        case SearchKind::Binary: binary(t, in.target); break;
        case SearchKind::Interpolation: interpolation(t, in.target); break;
        case SearchKind::Median: median(t); break;
        }
        return tl;
    }

private:
    static vector<int> range(int l, int r) {
        vector<int> v;
        for (int i = l; i <= r; i++) v.push_back(i);
        return v;
    }
    static void found(ArrayTrace<int>& t, int i, int line) {
        t.mark({i}, Role::Found, "Found " + to_string(t[i]) + " at index " + to_string(i) + ".", line);
    }
    static void notFound(ArrayTrace<int>& t, int target, int line) {
        t.highlight(range(0, t.size() - 1), Role::NotFound, to_string(target) + " is not in the array.", line);
    }

    void linear(ArrayTrace<int>& t, int target) {
        for (int i = 0; i < t.size(); i++) {
            t.pointer("i", i, "Look at index " + to_string(i) + ".", 0);
            bool eq = t[i] == target;
            t.compareValue(i, "Compare " + to_string(t[i]) + " with target " + to_string(target) + (eq ? ": equal." : ": not equal."), 1);
            if (eq) { found(t, i, 2); return; }
        }
        t.pointer("i", -1, "The end of the array was reached.", 3);
        notFound(t, target, 3);
    }

    void binary(ArrayTrace<int>& t, int target) {
        int lo = 0, hi = t.size() - 1;
        while (lo <= hi) {
            t.iterate("Search range is " + to_string(lo) + ".." + to_string(hi) + ".", 1);
            t.highlight(range(lo, hi), Role::Range, "Live search range [" + to_string(lo) + ", " + to_string(hi) + "].", 1);
            t.pointer("lo", lo, "lo = " + to_string(lo) + ".", 0);
            t.pointer("hi", hi, "hi = " + to_string(hi) + ".", 0);
            int mid = lo + (hi - lo) / 2;
            t.pointer("mid", mid, "mid = " + to_string(mid) + " (value " + to_string(t[mid]) + ").", 2);
            t.compareValue(mid, "Compare target " + to_string(target) + " with middle value " + to_string(t[mid]) + ".", 3);
            if (t[mid] == target) { found(t, mid, 3); return; }
            if (t[mid] < target) {
                t.mark(range(lo, mid), Role::Eliminated, "Target " + to_string(target) + " is greater than " + to_string(t[mid]) + ", so the left half can be discarded.", 4);
                lo = mid + 1;
            } else {
                t.mark(range(mid, hi), Role::Eliminated, "Target " + to_string(target) + " is smaller than " + to_string(t[mid]) + ", so the right half can be discarded.", 5);
                hi = mid - 1;
            }
        }
        notFound(t, target, 6);
    }

    void interpolation(ArrayTrace<int>& t, int target) {
        int lo = 0, hi = t.size() - 1;
        while (lo <= hi && target >= t[lo] && target <= t[hi]) {
            t.iterate("Search range is " + to_string(lo) + ".." + to_string(hi) + ".", 1);
            t.highlight(range(lo, hi), Role::Range, "Live search range [" + to_string(lo) + ", " + to_string(hi) + "].", 1);
            int pos = lo;
            if (t[hi] != t[lo]) pos = lo + (int)((long long)(target - t[lo]) * (hi - lo) / (t[hi] - t[lo]));
            t.rec.emit(OpType::Probe, {Delta::pointer("probe", pos)}, {t.tag(pos, Role::Current)},
                       "Estimated position = " + to_string(lo) + " + (" + to_string(target) + "-" + to_string(t[lo]) + ")*(" + to_string(hi - lo) +
                       ")/(" + to_string(t[hi] - t[lo]) + ") = " + to_string(pos) + ".", 2);
            t.compareValue(pos, "Compare target " + to_string(target) + " with " + to_string(t[pos]) + ".", 3);
            if (t[pos] == target) { found(t, pos, 3); return; }
            if (t[pos] < target) {
                t.mark(range(lo, pos), Role::Eliminated, "Target is greater, discard indices " + to_string(lo) + ".." + to_string(pos) + ".", 4);
                lo = pos + 1;
            } else {
                t.mark(range(pos, hi), Role::Eliminated, "Target is smaller, discard indices " + to_string(pos) + ".." + to_string(hi) + ".", 5);
                hi = pos - 1;
            }
        }
        if (lo <= hi) t.mark(range(lo, hi), Role::Eliminated, "Target lies outside the value range of the remaining elements.", 1);
        notFound(t, target, 6);
    }

    void median(ArrayTrace<int>& t) {
        int n = t.size();
        int k = (n - 1) / 2;
        int lo = 0, hi = n - 1;
        t.rec.note("The (lower) median is the element that would sit at index " + to_string(k) + " if the array were sorted.", 0);
        while (lo <= hi) {
            t.iterate("Active range " + to_string(lo) + ".." + to_string(hi) + ".", 1);
            int pivot = t[hi];
            vector<RoleTag> roles;
            for (int i = lo; i < hi; i++) roles.push_back(t.tag(i, Role::Range));
            roles.push_back(t.tag(hi, Role::Pivot));
            t.rec.emit(OpType::Partition, {Delta::pointer("pivot", hi)}, roles, "Partition around pivot " + to_string(pivot) + ".", 2);
            int store = lo;
            for (int j = lo; j < hi; j++) {
                bool less = t[j] < pivot;
                t.compare(j, hi, "Compare " + to_string(t[j]) + " with pivot " + to_string(pivot) + (less ? ": smaller, move it left." : ": not smaller."), 2);
                if (less) { if (store != j) t.swap(store, j, "Swap into the smaller-than-pivot region.", 2); store++; }
            }
            if (store != hi) t.swap(store, hi, "Put the pivot at its sorted position " + to_string(store) + ".", 2);
            t.pointer("pivot", -1, "Pivot is at index " + to_string(store) + ".", 2);
            if (store == k) { found(t, store, 3); return; }
            if (store < k) {
                t.mark(range(lo, store), Role::Eliminated, "Index " + to_string(store) + " < " + to_string(k) + ": the median is on the right.", 4);
                lo = store + 1;
            } else {
                t.mark(range(store, hi), Role::Eliminated, "Index " + to_string(store) + " > " + to_string(k) + ": the median is on the left.", 5);
                hi = store - 1;
            }
        }
    }

    SearchKind kind_;
    AlgorithmInfo info_;
};

inline void registerSearching(AlgorithmCatalog& cat) {
    for (SearchKind k : {SearchKind::Linear, SearchKind::Binary, SearchKind::Interpolation, SearchKind::Median}) {
        CatalogEntry e;
        AlgorithmInfo inf = searchInfo(k);
        e.id = inf.id; e.category = "Searching"; e.name = inf.name;
        e.input = k == SearchKind::Median ? InputKind::Array : InputKind::Search;
        e.structure = StructureKind::Array;
        e.make = [k] { return unique_ptr<Algorithm>(new SearchAlgorithm(k)); };
        cat.add(e);
    }
}

#endif
