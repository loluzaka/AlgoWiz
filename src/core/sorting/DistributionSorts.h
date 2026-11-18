#ifndef DISTRIBUTIONSORTS_H
#define DISTRIBUTIONSORTS_H

// Counting, Radix and Bucket sort (non-comparison sorts) plus registerSorting(),
// which adds all eight numeric sorts to the catalog.

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "ComparisonSorts.h"

using namespace std;

enum class DistSortKind { Counting, Radix, Bucket };

inline AlgorithmInfo distributionSortInfo(DistSortKind k) {
    AlgorithmInfo a;
    a.category = "Sorting";
    auto bindSource = [&](int line, const vector<OpType>& ops, const vector<string>& source) {
        for (OpType op : ops) addCppSourceBlock(a, line, op, "DistributionSorts.h", source);
    };
    switch (k) {
    case DistSortKind::Counting:
        a.id = "counting_sort"; a.name = "Counting Sort";
        a.best = "O(n + k)"; a.average = "O(n + k)"; a.worst = "O(n + k)"; a.space = "O(n + k)";
        a.description = "Counts how often each key occurs, accumulates the counts and places every value directly by key. k = value range.";
        a.pseudocode = {"count[key(a[i])]++ for every i", "for q = 1 .. k-1: count[q] += count[q-1]", "for i = n-1 down to 0:",
                        "  out[count[key]-1] = a[i]; count[key]--", "copy out[] back into a[]"};
        break;
    case DistSortKind::Radix:
        a.id = "radix_sort"; a.name = "Radix Sort (LSD)";
        a.best = "O(d(n + b))"; a.average = "O(d(n + b))"; a.worst = "O(d(n + b))"; a.space = "O(n + b)";
        a.description = "Sorts by one decimal digit at a time, least significant first, using 10 stable buckets. d = digits, b = 10.";
        a.pseudocode = {"for each digit position (1s, 10s, 100s ...):", "  distribute a[i] into bucket[digit]", "  collect buckets 0..9 back into a[]", "all digits done: sorted"};
        break;
    case DistSortKind::Bucket:
        a.id = "bucket_sort"; a.name = "Bucket Sort";
        a.best = "O(n + k)"; a.average = "O(n + k)"; a.worst = "O(n^2)"; a.space = "O(n + k)";
        a.description = "Spreads values over about sqrt(n) range buckets, sorts each bucket and concatenates them.";
        a.pseudocode = {"create nb empty buckets", "bucket[key * nb / (maxKey+1)].add(a[i]) for every i", "sort every bucket (insertion sort)", "write buckets 0..nb-1 back into a[]"};
        break;
    }
    switch (k) {
    case DistSortKind::Counting:
        bindSource(0, {OpType::Highlight, OpType::Assign}, {"for (int value : values) {", "    ++count[key(value)];", "}"});
        bindSource(1, {OpType::Assign}, {"for (int q = 1; q < count.size(); ++q) {", "    count[q] += count[q - 1];", "}"});
        bindSource(3, {OpType::Highlight, OpType::Assign}, {"for (int i = n - 1; i >= 0; --i) {", "    output[--count[key(values[i])]] = values[i];", "}"});
        bindSource(4, {OpType::Assign, OpType::Mark}, {"for (int i = 0; i < n; ++i) {", "    values[i] = output[i];", "}"});
        break;
    case DistSortKind::Radix:
        bindSource(0, {OpType::Iterate}, {"for (int exp = 1; maxKey / exp > 0; exp *= 10) {", "    distributeByDigit(values, exp);", "}"});
        bindSource(1, {OpType::Highlight, OpType::Assign}, {"int digit = (key(values[i]) / exp) % 10;", "buckets[digit].push_back(values[i]);", ""});
        bindSource(2, {OpType::Assign, OpType::Unmark}, {"for (int digit = 0; digit < 10; ++digit)", "    for (int value : buckets[digit]) values[k++] = value;", "clear(buckets);"});
        bindSource(3, {OpType::Mark}, {"}", "markSorted(values);", ""});
        break;
    case DistSortKind::Bucket:
        bindSource(0, {OpType::Note}, {"int bucketCount = max(1, (int)std::sqrt(n));", "std::vector<std::vector<int>> buckets(bucketCount);", ""});
        bindSource(1, {OpType::Highlight, OpType::Assign}, {"int bucket = key(values[i]) * bucketCount / (maxKey + 1);", "buckets[bucket].push_back(values[i]);", ""});
        bindSource(2, {OpType::Assign}, {"for (auto& bucket : buckets) {", "    std::stable_sort(bucket.begin(), bucket.end());", "}"});
        bindSource(3, {OpType::Assign, OpType::Mark}, {"for (const auto& bucket : buckets)", "    for (int value : bucket) values[k++] = value;", ""});
        break;
    }
    return a;
}

inline string joinInts(const vector<int>& v) {
    string s;
    for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += to_string(v[i]); }
    return s.empty() ? "-" : s;
}

class NumericDistributionSort : public Algorithm {
public:
    explicit NumericDistributionSort(DistSortKind k) : kind_(k), info_(distributionSortInfo(k)) {}
    const AlgorithmInfo& info() const override { return info_; }

    string validate(const Input& in) const override {
        if (in.values.empty()) return "";
        int mn = *min_element(in.values.begin(), in.values.end());
        int mx = *max_element(in.values.begin(), in.values.end());
        if ((long long)mx - mn > 99 && kind_ == DistSortKind::Counting)
            return "Counting sort needs a value range (max - min) of at most 99 so the count array fits on screen. Your range is " +
                   to_string((long long)mx - mn) + ".";
        if ((long long)mx - mn > 999999)
            return "Values are too spread out for this algorithm (range above 999999).";
        return "";
    }

    Timeline run(const Input& in) override {
        Timeline tl;
        tl.algorithm = info_.id;
        tl.structure = StructureKind::Array;
        tl.inputDescription = string(in.descending ? "descending " : "ascending ") + to_string(in.values.size()) + " values";
        Recorder rec(tl);
        int n = (int)in.values.size();
        if (n == 0) { tl.containerNames = {"Array"}; return tl; }
        int mn = *min_element(in.values.begin(), in.values.end());
        int mx = *max_element(in.values.begin(), in.values.end());
        bool desc = in.descending;
        auto key = [&](int v) { return desc ? mx - v : v - mn; };
        int maxKey = mx - mn;   // the key range is the same for both directions
        vector<int> all;
        for (int i = 0; i < n; i++) all.push_back(i);

        if (kind_ == DistSortKind::Counting) {
            tl.containerNames = {"Input array", "Count array (index = key)", "Output array"};
            int k = maxKey + 1;
            ArrayTrace<int> arr(rec, in.values, 0);
            ArrayTrace<int> cnt(rec, vector<int>(k, 0), 1);
            ArrayTrace<int> out(rec, vector<int>(n, 0), 2, false);
            arr.rec.note(string("Keys are ") + (desc ? "max - value" : "value - min") + ", so count[idx] stores how many values have key idx.", 0);
            for (int i = 0; i < n; i++) {
                int ky = key(arr[i]);
                arr.highlight({i}, Role::Current, "Read " + to_string(arr[i]) + " (key " + to_string(ky) + ").", 0);
                cnt.set(ky, cnt[ky] + 1, "count[" + to_string(ky) + "] becomes " + to_string(cnt[ky] + 1) + ".", 0);
            }
            for (int q = 1; q < k; q++)
                cnt.set(q, cnt[q] + cnt[q - 1], "Accumulate: count[" + to_string(q) + "] = count[" + to_string(q) + "] + count[" + to_string(q - 1) + "] = " + to_string(cnt[q] + cnt[q - 1]) + ".", 1);
            for (int i = n - 1; i >= 0; i--) {
                int ky = key(arr[i]);
                int pos = cnt[ky] - 1;
                arr.highlight({i}, Role::Current, "Take " + to_string(arr[i]) + " (key " + to_string(ky) + "): it belongs at output position " + to_string(pos) + ".", 3);
                out.set(pos, arr[i], "output[" + to_string(pos) + "] = " + to_string(arr[i]) + ".", 3);
                cnt.set(ky, cnt[ky] - 1, "count[" + to_string(ky) + "] decreases to " + to_string(cnt[ky] - 1) + ".", 3);
            }
            for (int i = 0; i < n; i++) arr.set(i, out[i], "Copy output[" + to_string(i) + "] = " + to_string(out[i]) + " back.", 4);
            arr.mark(all, Role::Sorted, "Counting sort finished.", 4);
        } else if (kind_ == DistSortKind::Radix) {
            tl.containerNames = {"Array", "Digit buckets 0-9"};
            ArrayTrace<int> arr(rec, in.values, 0);
            ArrayTrace<string> bk(rec, vector<string>(10, "-"), 1);
            int pass = 0;
            for (long long exp = 1; maxKey / exp > 0; exp *= 10, pass++) {
                arr.iterate("Pass " + to_string(pass + 1) + ": sort by the digit worth " + to_string(exp) + ".", 0);
                vector<vector<int>> cont(10);
                for (int i = 0; i < n; i++) {
                    int d = (int)((key(arr[i]) / exp) % 10);
                    arr.highlight({i}, Role::Current, "Value " + to_string(arr[i]) + " has digit " + to_string(d) + " at this position.", 1);
                    cont[d].push_back(arr[i]);
                    bk.set(d, joinInts(cont[d]), "Put " + to_string(arr[i]) + " in bucket " + to_string(d) + ".", 1);
                }
                int k = 0;
                for (int d = 0; d < 10; d++)
                    for (int v : cont[d]) { arr.set(k, v, "Collect " + to_string(v) + " from bucket " + to_string(d) + " into position " + to_string(k) + ".", 2); k++; }
                vector<Delta> reset;
                for (int d = 0; d < 10; d++) reset.push_back(Delta::valueText(d, "-", 1));
                rec.emit(OpType::Unmark, reset, {}, "Buckets emptied for the next pass.", 2);
            }
            arr.mark(all, Role::Sorted, "Radix sort finished after " + to_string(pass) + " pass(es).", 3);
        } else {
            tl.containerNames = {"Array", "Buckets (contents)"};
            int nb = max(1, (int)ceil(sqrt((double)n)));
            ArrayTrace<int> arr(rec, in.values, 0);
            ArrayTrace<string> bk(rec, vector<string>(nb, "-"), 1);
            vector<vector<int>> cont(nb);
            arr.rec.note("Using " + to_string(nb) + " buckets; bucket = key * " + to_string(nb) + " / " + to_string(maxKey + 1) + ".", 0);
            for (int i = 0; i < n; i++) {
                int b = (int)((long long)key(arr[i]) * nb / (maxKey + 1));
                arr.highlight({i}, Role::Current, "Value " + to_string(arr[i]) + " goes to bucket " + to_string(b) + ".", 1);
                cont[b].push_back(arr[i]);
                bk.set(b, joinInts(cont[b]), "Bucket " + to_string(b) + " now holds " + joinInts(cont[b]) + ".", 1);
            }
            for (int b = 0; b < nb; b++) {
                if (cont[b].size() < 2) continue;
                string before = joinInts(cont[b]);
                stable_sort(cont[b].begin(), cont[b].end(), [&](int x, int y) { return key(x) < key(y); });
                bk.set(b, joinInts(cont[b]), "Insertion-sort bucket " + to_string(b) + ": " + before + " -> " + joinInts(cont[b]) + ".", 2);
            }
            int k = 0;
            for (int b = 0; b < nb; b++)
                for (int v : cont[b]) { arr.set(k, v, "Write " + to_string(v) + " from bucket " + to_string(b) + " to position " + to_string(k) + ".", 3); k++; }
            arr.mark(all, Role::Sorted, "Bucket sort finished.", 3);
        }
        return tl;
    }
private:
    DistSortKind kind_;
    AlgorithmInfo info_;
};

inline void registerSorting(AlgorithmCatalog& cat) {
    auto addCmp = [&](CmpSortKind k) {
        CatalogEntry e;
        AlgorithmInfo inf = comparisonSortInfo(k);
        e.id = inf.id; e.category = "Sorting"; e.group = "Numeric sorting"; e.name = inf.name;
        e.input = InputKind::Array; e.structure = StructureKind::Array;
        e.make = [k] { return unique_ptr<Algorithm>(new NumericComparisonSort(k)); };
        cat.add(e);
    };
    addCmp(CmpSortKind::Bubble); addCmp(CmpSortKind::Selection); addCmp(CmpSortKind::Insertion);
    addCmp(CmpSortKind::Merge);  addCmp(CmpSortKind::Quick);
    for (DistSortKind k : {DistSortKind::Counting, DistSortKind::Radix, DistSortKind::Bucket}) {
        CatalogEntry e;
        AlgorithmInfo inf = distributionSortInfo(k);
        e.id = inf.id; e.category = "Sorting"; e.group = "Numeric sorting"; e.name = inf.name;
        e.input = InputKind::Array; e.structure = StructureKind::Array;
        e.make = [k] { return unique_ptr<Algorithm>(new NumericDistributionSort(k)); };
        cat.add(e);
    }
}

#endif
