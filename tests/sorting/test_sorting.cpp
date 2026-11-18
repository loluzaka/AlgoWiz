// Tests for src/core/sorting (M3): every numeric sort, ascending and descending,
// over the section-36 input shapes (empty, 1, 2, sorted, reverse, duplicates, negatives).
#include <algorithm>

#include "../TestFramework.h"
#include "../../src/core/sorting/DistributionSorts.h"

static AlgorithmCatalog& sortCatalog() {
    static AlgorithmCatalog cat;
    if (cat.all().empty()) registerSorting(cat);
    return cat;
}

TEST_CASE(sorting_catalog_has_eight_algorithms) {
    CHECK_EQ(sortCatalog().inCategory("Sorting").size(), (size_t)8);
}

TEST_CASE(sorting_all_algorithms_sort_correctly) {
    for (const auto& entry : sortCatalog().all()) {
        for (bool desc : {false, true}) {
            for (const auto& arr : tf::sampleArrays()) {
                auto algo = entry.make();
                Input in; in.values = arr; in.descending = desc;
                Timeline tl = generate(*algo, in);
                if (!tl.error.empty()) { CHECK(entry.id == "counting_sort"); continue; }
                tf::verifyTimeline(tl);
                std::vector<int> expect = arr;
                std::sort(expect.begin(), expect.end());
                if (desc) std::reverse(expect.begin(), expect.end());
                WorldState w = tf::finalState(tl);
                std::vector<int> got = tf::intsOf(w, 0);
                if (!(got == expect)) std::cerr << "  " << entry.id << (desc ? " desc " : " asc ") << tf::join(arr) << " -> " << tf::join(got) << "\n";
                CHECK(got == expect);
            }
        }
    }
}

TEST_CASE(sorting_empty_input_has_no_steps_for_comparison_sorts) {
    for (const char* id : {"bubble_sort", "selection_sort", "insertion_sort", "merge_sort", "quick_sort"}) {
        auto algo = sortCatalog().find(id)->make();
        Input in;
        Timeline tl = generate(*algo, in);
        CHECK(tl.error.empty());
        CHECK_EQ(tl.steps.size(), (size_t)0);
    }
}

TEST_CASE(sorting_bubble_stops_early_on_sorted_input) {
    auto algo = sortCatalog().find("bubble_sort")->make();
    Input in; in.values = {1, 2, 3, 4, 5};
    Timeline tl = generate(*algo, in);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().comparisons, 4L);   // exactly one pass
    CHECK_EQ(e.stats().swaps, 0L);
}

TEST_CASE(sorting_bubble_counts_match_known_values) {
    auto algo = sortCatalog().find("bubble_sort")->make();
    Input in; in.values = {5, 1, 4, 2, 8};
    Timeline tl = generate(*algo, in);
    ExecutionEngine e; e.load(tl); e.toEnd();
    CHECK_EQ(e.stats().swaps, 4L);         // 4 inversions
}

TEST_CASE(sorting_counting_rejects_huge_range) {
    auto algo = sortCatalog().find("counting_sort")->make();
    Input in; in.values = {1, 100000};
    Timeline tl = generate(*algo, in);
    CHECK(!tl.error.empty());
    CHECK(tl.steps.empty());
}

TEST_CASE(sorting_merge_uses_temporary_row) {
    auto algo = sortCatalog().find("merge_sort")->make();
    Input in; in.values = {4, 3, 2, 1};
    Timeline tl = generate(*algo, in);
    bool usesAux = false;
    for (const auto& s : tl.steps) for (const auto& d : s.deltas) if (d.container == 1) usesAux = true;
    CHECK(usesAux);
    WorldState w = tf::finalState(tl);
    const Container* aux = w.find(1);
    CHECK(aux != nullptr && aux->nodes.empty());      // cleared after the final merge
}

TEST_CASE(sorting_every_step_has_valid_code_line) {
    for (const auto& entry : sortCatalog().all()) {
        auto algo = entry.make();
        Input in; in.values = {4, 2, 7, 1, 9, 3};
        Timeline tl = generate(*algo, in);
        int lines = (int)algo->info().pseudocode.size();
        for (const auto& s : tl.steps) CHECK(s.codeLine < lines);
    }
}
