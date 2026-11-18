#ifndef COMPARISONSORTS_H
#define COMPARISONSORTS_H

// Bubble, Selection, Insertion, Merge and Quick sort written ONCE as templates.
// They work on ArrayTrace<T>, so the same code sorts numbers and (in StringSorts.h) words.

#include <functional>
#include <string>
#include <vector>

#include "../../engine/Algorithm.h"
#include "../../engine/Recorder.h"
#include "../AlgorithmCatalog.h"

using namespace std;

// Ordering policy. before(x,y) == true means "x must come before y" (strict).
template <class T>
struct SortCmp {
    function<bool(const T&, const T&)> before;
    function<string(const T&, const T&)> detail;   // optional extra narration (character comparison)
};

template <class T>
string explainPair(const SortCmp<T>& c, const T& x, const T& y, bool outOfOrder) {
    string s = "Comparing " + show(x) + " and " + show(y) + ". ";
    s += outOfOrder ? show(x) + " must come after " + show(y) + ", so they are out of order."
                    : show(x) + " already comes before (or ties with) " + show(y) + ", so no change.";
    if (c.detail) { string d = c.detail(x, y); if (!d.empty()) s += " [" + d + "]"; }
    return s;
}

// -------------------------------------------------------------------- Bubble
template <class T>
void bubbleSort(ArrayTrace<T>& t, const SortCmp<T>& c) {
    int n = t.size();
    vector<bool> done(n, false);
    for (int i = 0; i < n - 1; i++) {
        t.iterate("Pass " + to_string(i + 1) + ": bubble the next extreme value to position " + to_string(n - 1 - i) + ".", 0);
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; j++) {
            bool out = c.before(t[j + 1], t[j]);
            t.compare(j, j + 1, explainPair(c, t[j], t[j + 1], out), 2);
            if (out) { t.swap(j, j + 1, "Swap " + show(t[j]) + " and " + show(t[j + 1]) + ".", 3); swapped = true; }
        }
        t.mark({n - 1 - i}, Role::Sorted, "Position " + to_string(n - 1 - i) + " is now fixed.", 4);
        done[n - 1 - i] = true;
        if (!swapped) {
            vector<int> rest;
            for (int k = 0; k < n; k++) if (!done[k]) { rest.push_back(k); done[k] = true; }
            if (!rest.empty()) t.mark(rest, Role::Sorted, "No swaps in this pass: the array is already sorted.", 5);
            return;
        }
    }
    if (n > 0 && !done[0]) t.mark({0}, Role::Sorted, "The last remaining element is in place.", 4);
}

// ----------------------------------------------------------------- Selection
template <class T>
void selectionSort(ArrayTrace<T>& t, const SortCmp<T>& c) {
    int n = t.size();
    for (int i = 0; i < n - 1; i++) {
        t.iterate("Pass " + to_string(i + 1) + ": find the extreme value of positions " + to_string(i) + ".." + to_string(n - 1) + ".", 0);
        int m = i;
        t.pointer("min", m, "Assume position " + to_string(m) + " (" + show(t[m]) + ") is the current best.", 1);
        for (int j = i + 1; j < n; j++) {
            bool better = c.before(t[j], t[m]);
            t.compare(j, m, "Comparing " + show(t[j]) + " with the current best " + show(t[m]) + (better ? ": it is better." : ": no change."), 3);
            if (better) { m = j; t.pointer("min", m, "New best is position " + to_string(m) + " (" + show(t[m]) + ").", 4); }
        }
        if (m != i) t.swap(i, m, "Swap the best (" + show(t[m]) + ") into position " + to_string(i) + ".", 5);
        else t.highlight({i}, Role::Active, "Position " + to_string(i) + " already holds the best value.", 5);
        t.mark({i}, Role::Sorted, "Position " + to_string(i) + " is fixed.", 6);
    }
    if (n > 0) {
        t.pointer("min", -1, "Selection sort finished.", 6);
        t.mark({n - 1}, Role::Sorted, "The last element is in place.", 6);
    }
}

// ----------------------------------------------------------------- Insertion
template <class T>
void insertionSort(ArrayTrace<T>& t, const SortCmp<T>& c) {
    int n = t.size();
    if (n > 0) t.mark({0}, Role::Sorted, "A single element is a sorted prefix.", 0);
    for (int i = 1; i < n; i++) {
        T key = t[i];
        t.iterate("Insert " + show(key) + " into the sorted prefix.", 1);
        t.pointer("key", i, "key = " + show(key) + " (position " + to_string(i) + ").", 2);
        int j = i - 1;
        while (j >= 0) {
            bool shift = c.before(key, t[j]);
            t.compareValue(j, "Compare " + show(t[j]) + " with key " + show(key) + (shift ? ": it must move right." : ": key belongs after it."), 4);
            if (!shift) break;
            t.set(j + 1, t[j], "Shift " + show(t[j]) + " one place right.", 5);
            j--;
        }
        t.set(j + 1, key, "Place key " + show(key) + " at position " + to_string(j + 1) + ".", 6);
        vector<int> prefix;
        for (int k = 0; k <= i; k++) prefix.push_back(k);
        t.mark(prefix, Role::Sorted, "Positions 0.." + to_string(i) + " are sorted.", 7);
    }
    if (n > 1) t.pointer("key", -1, "Insertion sort finished.", 7);
}

// --------------------------------------------------------------------- Merge
template <class T>
class MergeSorter {
public:
    MergeSorter(ArrayTrace<T>& main, ArrayTrace<T>& aux, const SortCmp<T>& c) : t(main), x(aux), cmp(c) {}
    void sort(int l, int r) {
        if (l >= r) return;
        int mid = (l + r) / 2;
        vector<RoleTag> roles;
        for (int i = l; i <= mid; i++) roles.push_back(t.tag(i, Role::GroupA));
        for (int i = mid + 1; i <= r; i++) roles.push_back(t.tag(i, Role::GroupB));
        t.rec.emit(OpType::Highlight, {}, roles,
                   "Split positions " + to_string(l) + ".." + to_string(r) + " into [" + to_string(l) + ".." + to_string(mid) +
                   "] and [" + to_string(mid + 1) + ".." + to_string(r) + "].", 1);
        sort(l, mid);
        sort(mid + 1, r);
        merge(l, mid, r);
    }
private:
    void merge(int l, int mid, int r) {
        vector<Delta> copy;
        for (int i = l; i <= r; i++) copy.push_back(valueDelta(i, t[i], 1));
        vector<RoleTag> roles;
        for (int i = l; i <= mid; i++) roles.push_back(t.tag(i, Role::GroupA));
        for (int i = mid + 1; i <= r; i++) roles.push_back(t.tag(i, Role::GroupB));
        x.loadRange(l, r, t.data());   // keep the aux shadow vector in sync
        t.rec.emit(OpType::Merge, copy, roles,
                   "Merge [" + to_string(l) + ".." + to_string(mid) + "] and [" + to_string(mid + 1) + ".." + to_string(r) +
                   "]: copy both runs into the temporary row.", 3);
        int i = l, j = mid + 1, k = l;
        while (i <= mid && j <= r) {
            bool takeRight = cmp.before(x[j], x[i]);
            x.compare(i, j, "Compare " + show(x[i]) + " (left run) with " + show(x[j]) + " (right run): take " +
                      (takeRight ? "the right one." : "the left one."), 4);
            if (takeRight) { t.set(k, x[j], "Write " + show(x[j]) + " to position " + to_string(k) + ".", 5); j++; }
            else           { t.set(k, x[i], "Write " + show(x[i]) + " to position " + to_string(k) + ".", 5); i++; }
            k++;
        }
        while (i <= mid) { t.set(k, x[i], "Left run leftover: write " + show(x[i]) + " to position " + to_string(k) + ".", 6); k++; i++; }
        while (j <= r)   { t.set(k, x[j], "Right run leftover: write " + show(x[j]) + " to position " + to_string(k) + ".", 6); k++; j++; }
        vector<Delta> drop;
        for (int q = l; q <= r; q++) drop.push_back(Delta::destroy(q, 1));
        t.rec.emit(OpType::Merge, drop, {}, "Positions " + to_string(l) + ".." + to_string(r) + " are merged; temporary row cleared.", 6);
    }
    ArrayTrace<T>& t;
    ArrayTrace<T>& x;
    const SortCmp<T>& cmp;
};

template <class T>
void mergeSort(ArrayTrace<T>& t, ArrayTrace<T>& aux, const SortCmp<T>& c) {
    int n = t.size();
    if (n == 0) return;
    MergeSorter<T> m(t, aux, c);
    m.sort(0, n - 1);
    vector<int> all;
    for (int i = 0; i < n; i++) all.push_back(i);
    t.mark(all, Role::Sorted, "Merge sort finished.", 6);
}

// ---------------------------------------------------------------------- Quick
template <class T>
class QuickSorter {
public:
    QuickSorter(ArrayTrace<T>& tr, const SortCmp<T>& c) : t(tr), cmp(c) {}
    void sort(int l, int r) {
        if (l > r) return;
        if (l == r) { t.mark({l}, Role::Sorted, "A one-element range is sorted.", 0); return; }
        int p = partition(l, r);
        sort(l, p - 1);
        sort(p + 1, r);
    }
private:
    int partition(int l, int r) {
        T pivot = t[r];
        vector<RoleTag> roles;
        for (int i = l; i < r; i++) roles.push_back(t.tag(i, Role::Range));
        roles.push_back(t.tag(r, Role::Pivot));
        t.rec.emit(OpType::Partition, {Delta::pointer("pivot", r, 0)}, roles,
                   "Partition positions " + to_string(l) + ".." + to_string(r) + " around pivot " + show(pivot) + " (last element).", 1);
        int i = l - 1;
        t.pointer("store", i, "Boundary of the 'before pivot' region starts at " + to_string(i) + ".", 2);
        for (int j = l; j < r; j++) {
            bool lessP = cmp.before(t[j], pivot);
            t.compare(j, r, "Compare " + show(t[j]) + " with pivot " + show(pivot) + (lessP ? ": it goes before the pivot." : ": it stays after the pivot."), 3);
            if (lessP) {
                i++;
                if (i != j) t.swap(i, j, "Swap " + show(t[i]) + " into the before-pivot region.", 4);
                t.pointer("store", i, "Before-pivot region now ends at " + to_string(i) + ".", 4);
            }
        }
        if (i + 1 != r) t.swap(i + 1, r, "Put the pivot into its final position " + to_string(i + 1) + ".", 5);
        t.mark({i + 1}, Role::Sorted, "Pivot " + show(pivot) + " is fixed at position " + to_string(i + 1) + ".", 5);
        t.pointer("store", -1, "Partition done.", 5);
        t.pointer("pivot", -1, "Recurse on both sides.", 6);
        return i + 1;
    }
    ArrayTrace<T>& t;
    const SortCmp<T>& cmp;
};

template <class T>
void quickSort(ArrayTrace<T>& t, const SortCmp<T>& c) {
    QuickSorter<T> q(t, c);
    q.sort(0, t.size() - 1);
}

// -------------------------------------------------------------- Algorithm class
enum class CmpSortKind { Bubble, Selection, Insertion, Merge, Quick };

inline AlgorithmInfo comparisonSortInfo(CmpSortKind k) {
    AlgorithmInfo a;
    a.category = "Sorting";
    auto bindSource = [&](int line, const vector<OpType>& ops, const vector<string>& source) {
        for (OpType op : ops) addCppSourceBlock(a, line, op, "ComparisonSorts.h", source);
    };
    switch (k) {
    case CmpSortKind::Bubble:
        a.id = "bubble_sort"; a.name = "Bubble Sort";
        a.best = "O(n)"; a.average = "O(n^2)"; a.worst = "O(n^2)"; a.space = "O(1)";
        a.description = "Repeatedly swaps adjacent out-of-order elements; each pass fixes one value at the end.";
        a.pseudocode = {"for i = 0 .. n-2:", "  swapped = false", "  for j = 0 .. n-i-2:", "    if a[j] > a[j+1]: swap(a[j], a[j+1]); swapped = true",
                        "  a[n-i-1] is now sorted", "  if not swapped: stop"};
        break;
    case CmpSortKind::Selection:
        a.id = "selection_sort"; a.name = "Selection Sort";
        a.best = "O(n^2)"; a.average = "O(n^2)"; a.worst = "O(n^2)"; a.space = "O(1)";
        a.description = "Finds the smallest remaining value and swaps it into the next position.";
        a.pseudocode = {"for i = 0 .. n-2:", "  min = i", "  for j = i+1 .. n-1:", "    if a[j] < a[min]:", "      min = j", "  swap(a[i], a[min])", "  a[i] is sorted"};
        break;
    case CmpSortKind::Insertion:
        a.id = "insertion_sort"; a.name = "Insertion Sort";
        a.best = "O(n)"; a.average = "O(n^2)"; a.worst = "O(n^2)"; a.space = "O(1)";
        a.description = "Grows a sorted prefix by inserting each new key at its correct place, shifting larger values right.";
        a.pseudocode = {"a[0] is sorted", "for i = 1 .. n-1:", "  key = a[i]; j = i-1", "  while j >= 0:", "    if key < a[j]:", "      a[j+1] = a[j]; j--", "  a[j+1] = key", "  prefix 0..i is sorted"};
        break;
    case CmpSortKind::Merge:
        a.id = "merge_sort"; a.name = "Merge Sort";
        a.best = "O(n log n)"; a.average = "O(n log n)"; a.worst = "O(n log n)"; a.space = "O(n)";
        a.description = "Splits the range in halves, sorts each recursively and merges the two sorted runs.";
        a.pseudocode = {"mergeSort(l, r):", "  if l >= r: return; mid = (l+r)/2", "  mergeSort(l, mid); mergeSort(mid+1, r)", "  copy a[l..r] to temp",
                        "  while both runs non-empty: compare heads", "    write the smaller to a[k++]", "  copy leftovers; merge done"};
        break;
    case CmpSortKind::Quick:
        a.id = "quick_sort"; a.name = "Quick Sort";
        a.best = "O(n log n)"; a.average = "O(n log n)"; a.worst = "O(n^2)"; a.space = "O(log n)";
        a.description = "Partitions around a pivot (last element, Lomuto scheme) and recurses on both sides.";
        a.pseudocode = {"quickSort(l, r):", "  pivot = a[r]; i = l-1", "  (Lomuto partition)", "  for j = l .. r-1: if a[j] < pivot:", "    i++; swap(a[i], a[j])",
                        "  swap(a[i+1], a[r])", "  quickSort(l, p-1); quickSort(p+1, r)"};
        break;
    }
    switch (k) {
    case CmpSortKind::Bubble:
        bindSource(0, {OpType::Iterate}, {"for (int pass = 0; pass < n - 1; ++pass) {", "    bool swapped = false;", "}"});
        bindSource(2, {OpType::Compare}, {"for (int j = 0; j < n - 1 - pass; ++j) {", "    if (values[j] > values[j + 1]) {", "}"});
        bindSource(3, {OpType::Swap}, {"if (values[j] > values[j + 1]) {", "    std::swap(values[j], values[j + 1]);", "    swapped = true;"});
        bindSource(4, {OpType::Mark}, {"}", "fixed[n - 1 - pass] = true;", "if (!swapped) break;"});
        bindSource(5, {OpType::Mark}, {"if (!swapped) {", "    break;", "}"});
        break;
    case CmpSortKind::Selection:
        bindSource(0, {OpType::Iterate}, {"for (int i = 0; i < n - 1; ++i) {", "    int best = i;", "}"});
        bindSource(1, {OpType::PointerMove}, {"int best = i;", "for (int j = i + 1; j < n; ++j) {", "    if (values[j] < values[best]) best = j;"});
        bindSource(3, {OpType::Compare}, {"for (int j = i + 1; j < n; ++j) {", "    if (values[j] < values[best]) {", "        best = j;"});
        bindSource(4, {OpType::PointerMove}, {"if (values[j] < values[best]) {", "    best = j;", "}"});
        bindSource(5, {OpType::Swap, OpType::Highlight}, {"if (best != i)", "    std::swap(values[i], values[best]);", "else"});
        bindSource(6, {OpType::Mark, OpType::PointerMove}, {"std::swap(values[i], values[best]);", "sorted[i] = true;", "}"});
        break;
    case CmpSortKind::Insertion:
        bindSource(0, {OpType::Mark}, {"if (n > 0) {", "    sorted[0] = true;", "}"});
        bindSource(1, {OpType::Iterate}, {"for (int i = 1; i < n; ++i) {", "    int key = values[i];", "}"});
        bindSource(2, {OpType::PointerMove}, {"int key = values[i];", "int j = i - 1;", "while (j >= 0 && values[j] > key) {"});
        bindSource(4, {OpType::Compare}, {"while (j >= 0) {", "    if (values[j] <= key) break;", "}"});
        bindSource(5, {OpType::Assign}, {"while (j >= 0 && values[j] > key) {", "    values[j + 1] = values[j];", "    --j;"});
        bindSource(6, {OpType::Assign}, {"}", "values[j + 1] = key;", ""});
        bindSource(7, {OpType::Mark, OpType::PointerMove}, {"values[j + 1] = key;", "markSortedPrefix(i);", "}"});
        break;
    case CmpSortKind::Merge:
        bindSource(1, {OpType::Highlight}, {"if (left >= right) return;", "int middle = left + (right - left) / 2;", "mergeSort(left, middle);"});
        bindSource(3, {OpType::Merge}, {"std::copy(values.begin() + left, values.begin() + right + 1, temp.begin() + left);", "int i = left, j = middle + 1, k = left;", "while (i <= middle && j <= right) {"});
        bindSource(4, {OpType::Compare}, {"while (i <= middle && j <= right) {", "    if (temp[i] <= temp[j])", "        values[k++] = temp[i++];"});
        bindSource(5, {OpType::Assign}, {"if (temp[i] <= temp[j])", "    values[k++] = temp[i++];", "else"});
        bindSource(6, {OpType::Assign, OpType::Merge, OpType::Mark}, {"while (i <= middle) values[k++] = temp[i++];", "while (j <= right) values[k++] = temp[j++];", "}"});
        break;
    case CmpSortKind::Quick:
        bindSource(0, {OpType::Mark}, {"if (left >= right) return;", "int pivotIndex = partition(left, right);", "quickSort(left, pivotIndex - 1);"});
        bindSource(1, {OpType::Partition}, {"int pivot = values[right];", "int boundary = left - 1;", "for (int j = left; j < right; ++j) {"});
        bindSource(2, {OpType::PointerMove}, {"int pivot = values[right];", "int boundary = left - 1;", "for (int j = left; j < right; ++j) {"});
        bindSource(3, {OpType::Compare}, {"for (int j = left; j < right; ++j) {", "    if (values[j] < pivot) {", "        ++boundary;"});
        bindSource(4, {OpType::Swap, OpType::PointerMove}, {"if (values[j] < pivot) {", "    ++boundary;", "    std::swap(values[boundary], values[j]);"});
        bindSource(5, {OpType::Swap, OpType::Mark, OpType::PointerMove}, {"}", "std::swap(values[boundary + 1], values[right]);", "return boundary + 1;"});
        bindSource(6, {OpType::PointerMove}, {"quickSort(left, pivotIndex - 1);", "quickSort(pivotIndex + 1, right);", "}"});
        break;
    }
    return a;
}

class NumericComparisonSort : public Algorithm {
public:
    explicit NumericComparisonSort(CmpSortKind k) : kind_(k), info_(comparisonSortInfo(k)) {}
    const AlgorithmInfo& info() const override { return info_; }
    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = info_.id;
        tl.structure = StructureKind::Array;
        tl.containerNames = {"Array", "Temporary"};
        tl.inputDescription = string(in.descending ? "descending " : "ascending ") + to_string(in.values.size()) + " values";
        Recorder rec(tl);
        SortCmp<int> cmp;
        bool desc = in.descending;
        cmp.before = [desc](const int& x, const int& y) { return desc ? x > y : x < y; };
        ArrayTrace<int> t(rec, in.values, 0);
        switch (kind_) {
        case CmpSortKind::Bubble:    bubbleSort(t, cmp); break;
        case CmpSortKind::Selection: selectionSort(t, cmp); break;
        case CmpSortKind::Insertion: insertionSort(t, cmp); break;
        case CmpSortKind::Quick:     quickSort(t, cmp); break;
        case CmpSortKind::Merge: {
            ArrayTrace<int> aux(rec, in.values, 1, false);
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

#endif
