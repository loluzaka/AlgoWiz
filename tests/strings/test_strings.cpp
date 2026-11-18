// Tests for src/core/strings (M3): the six string-sorting criteria and the
// character-by-character comparison text.
#include <algorithm>

#include "../TestFramework.h"
#include "../../src/core/strings/StringSorts.h"

static vector<string> runStrings(CmpSortKind kind, vector<string> words, StringMode mode, bool ci, Timeline* out = nullptr) {
    StringSort algo(kind);
    Input in; in.words = words; in.stringMode = mode; in.caseInsensitive = ci;
    Timeline tl = generate(algo, in);
    if (out) *out = tl;
    CHECK(tl.error.empty());
    tf::verifyTimeline(tl);
    return tf::textsOf(tf::finalState(tl), 0);
}

static const CmpSortKind ALL[] = {CmpSortKind::Bubble, CmpSortKind::Selection, CmpSortKind::Insertion, CmpSortKind::Merge, CmpSortKind::Quick};

TEST_CASE(strings_lexicographic_ascending_all_algorithms) {
    for (CmpSortKind k : ALL) {
        auto r = runStrings(k, {"banana", "apple", "mango", "cherry"}, StringMode::Lexicographic, false);
        CHECK(r == (vector<string>{"apple", "banana", "cherry", "mango"}));
    }
}

TEST_CASE(strings_reverse_lexicographic) {
    for (CmpSortKind k : ALL) {
        auto r = runStrings(k, {"banana", "apple", "mango", "cherry"}, StringMode::LexicographicReverse, false);
        CHECK(r == (vector<string>{"mango", "cherry", "banana", "apple"}));
    }
}

TEST_CASE(strings_case_sensitive_puts_uppercase_first) {
    auto r = runStrings(CmpSortKind::Insertion, {"banana", "Apple", "cherry", "apple"}, StringMode::Lexicographic, false);
    CHECK(r == (vector<string>{"Apple", "apple", "banana", "cherry"}));
}

TEST_CASE(strings_case_insensitive_ignores_case) {
    auto r = runStrings(CmpSortKind::Bubble, {"Banana", "apple", "Cherry", "Apple"}, StringMode::Lexicographic, true);
    CHECK_EQ(r.size(), (size_t)4);
    CHECK(foldCase(r[0]) == "apple" && foldCase(r[1]) == "apple" && foldCase(r[2]) == "banana" && foldCase(r[3]) == "cherry");
}

TEST_CASE(strings_length_ascending_and_descending) {
    for (CmpSortKind k : ALL) {
        auto asc = runStrings(k, {"banana", "apple", "mango", "cherry"}, StringMode::Length, false);
        for (size_t i = 1; i < asc.size(); i++) CHECK(asc[i - 1].size() <= asc[i].size());
        auto desc = runStrings(k, {"banana", "apple", "mango", "cherry"}, StringMode::LengthReverse, false);
        for (size_t i = 1; i < desc.size(); i++) CHECK(desc[i - 1].size() >= desc[i].size());
    }
}

TEST_CASE(strings_edge_cases_empty_one_two) {
    for (CmpSortKind k : ALL) {
        CHECK(runStrings(k, {}, StringMode::Lexicographic, false).empty());
        CHECK(runStrings(k, {"solo"}, StringMode::Lexicographic, false) == (vector<string>{"solo"}));
        CHECK(runStrings(k, {"b", "a"}, StringMode::Lexicographic, false) == (vector<string>{"a", "b"}));
    }
}

TEST_CASE(strings_character_comparison_text) {
    CHECK_EQ(characterComparison("apple", "apricot", false), string("a = a, p = p, p < r: \"apple\" sorts before \"apricot\""));
    CHECK(characterComparison("ab", "abc", false).find("prefix") != string::npos);
    CHECK(characterComparison("Ab", "ab", true).find("identical") != string::npos);
}

TEST_CASE(strings_compare_notes_show_letters) {
    Timeline tl;
    runStrings(CmpSortKind::Bubble, {"apple", "apricot"}, StringMode::Lexicographic, false, &tl);
    bool found = false;
    for (const auto& s : tl.steps) if (s.op == OpType::Compare && s.note.find("p < r") != string::npos) found = true;
    CHECK(found);
}

TEST_CASE(strings_validation_rejects_bad_words) {
    StringSort algo(CmpSortKind::Bubble);
    Input in; in.words = {"ok", ""};
    CHECK(!generate(algo, in).error.empty());
    Input in2; in2.words = {"averyveryverylongword"};
    CHECK(!generate(algo, in2).error.empty());
}
