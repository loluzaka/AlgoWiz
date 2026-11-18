// Tests for src/engine/race, quiz and report (M1): Race Mode lockstep logic, comparison table,
// Quiz Mode question building and scoring, Report Export Markdown.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>

#include "../TestFramework.h"
#include "../../src/core/BuiltinCatalog.h"
#include "../../src/engine/quiz/QuizController.h"
#include "../../src/engine/race/RaceController.h"
#include "../../src/engine/report/ReportExporter.h"

static AlgorithmCatalog& builtin() {
    static AlgorithmCatalog cat = buildBuiltinCatalog();
    return cat;
}
static Timeline runBuiltin(const string& id, vector<int> values) {
    auto algo = builtin().find(id)->make();
    Input in; in.values = values;
    return generate(*algo, in);
}

TEST_CASE(race_identical_inputs_finish_with_identical_results) {
    RaceController race;
    race.load(runBuiltin("bubble_sort", {8, 3, 5, 1, 9, 2}), runBuiltin("quick_sort", {8, 3, 5, 1, 9, 2}), "Bubble", "Quick");
    int ticks = 0;
    while (race.tick()) ticks++;
    CHECK(race.finished());
    CHECK(tf::intsOf(race.paneA().state()) == tf::intsOf(race.paneB().state()));
    CHECK_EQ((size_t)ticks, max(race.paneA().total(), race.paneB().total()));     // Ticks mode: slower algorithm sets the length
    CHECK(!race.tick());
}

TEST_CASE(race_unequal_step_counts_in_ticks_mode) {
    RaceController race;
    race.load(runBuiltin("bubble_sort", {9, 8, 7, 6, 5, 4, 3, 2, 1}), runBuiltin("merge_sort", {9, 8, 7, 6, 5, 4, 3, 2, 1}), "A", "B");
    CHECK(race.fewerSteps() != 0);
    size_t shorter = min(race.paneA().total(), race.paneB().total());
    for (size_t i = 0; i < shorter; i++) race.tick();
    CHECK(race.paneA().atEnd() != race.paneB().atEnd());       // exactly one pane has finished
    CHECK(!race.finished());
}

TEST_CASE(race_progress_mode_keeps_panes_at_equal_fraction) {
    RaceController race;
    race.load(runBuiltin("bubble_sort", {9, 8, 7, 6, 5, 4, 3, 2, 1}), runBuiltin("insertion_sort", {9, 8, 7, 6, 5, 4, 3, 2, 1}), "A", "B");
    race.setMode(RaceMode::Progress);
    int guard = 0;
    while (!race.finished() && guard++ < 100000) {
        race.tick();
        CHECK(std::fabs(race.fractionA() - race.fractionB()) < 0.2);
    }
    CHECK(race.finished());
}

TEST_CASE(race_back_and_reset_and_empty_timeline) {
    RaceController race;
    race.load(runBuiltin("bubble_sort", {3, 2, 1}), runBuiltin("bubble_sort", {}), "A", "B");     // pane B has zero steps
    CHECK(race.paneB().atEnd());
    race.tick(); race.tick(); race.tick();
    size_t posA = race.paneA().position();
    CHECK(race.back());
    CHECK(race.paneA().position() == posA - 1);
    race.reset();
    CHECK(race.paneA().atStart() && race.ticks() == 0);
    CHECK(!race.back());
}

TEST_CASE(comparison_table_lists_every_algorithm_with_metrics) {
    vector<Timeline> t = {runBuiltin("bubble_sort", {8, 3, 5, 1, 9, 2}), runBuiltin("selection_sort", {8, 3, 5, 1, 9, 2}), runBuiltin("insertion_sort", {8, 3, 5, 1, 9, 2})};
    auto rows = compareTimelines(t, {"Bubble Sort", "Selection Sort", "Insertion Sort"});
    CHECK_EQ(rows.size(), (size_t)3);
    CHECK_EQ(rows[0].stats.comparisons, 15L);               // bubble sort without early exit on this input
    CHECK_EQ(rows[1].stats.comparisons, 15L);               // n(n-1)/2
    string table = comparisonTable(rows);
    CHECK(table.find("Selection Sort") != string::npos);
    CHECK(table.find("Comparisons") != string::npos);
}

TEST_CASE(quiz_questions_are_valid_and_scoring_works) {
    QuizController quiz(runBuiltin("bubble_sort", {4, 3, 2, 1}), 99);
    int asked = 0;
    while (!quiz.finished()) {
        QuizQuestion q = quiz.ask();
        CHECK(q.options.size() >= 2 && q.options.size() <= 4);
        CHECK(q.correct >= 0 && q.correct < (int)q.options.size());
        set<string> uniq(q.options.begin(), q.options.end());
        CHECK_EQ(uniq.size(), q.options.size());                          // no duplicate options
        string expected = describeStep(*quiz.engine().nextStep());
        CHECK_EQ(q.options[q.correct], expected);
        bool wrongOnPurpose = asked % 3 == 2;
        bool ok = quiz.answer(q, wrongOnPurpose ? (q.correct + 1) % (int)q.options.size() : q.correct);
        CHECK_EQ(ok, !wrongOnPurpose);
        asked++;
    }
    CHECK_EQ(quiz.score().answered(), asked);
    CHECK(quiz.score().correct() < asked);
    CHECK(quiz.score().bestStreak() >= 2);
    CHECK(quiz.score().percent() > 0.0 && quiz.score().percent() < 100.0);
    CHECK(!quiz.score().summary().empty());
}

TEST_CASE(quiz_score_persists_across_restart_and_handles_empty) {
    QuizController quiz(runBuiltin("bubble_sort", {2, 1}), 5);
    QuizQuestion q = quiz.ask();
    quiz.answer(q, q.correct);
    int before = quiz.score().answered();
    quiz.restart();
    CHECK_EQ(quiz.score().answered(), before);
    QuizController empty(runBuiltin("bubble_sort", {}));
    CHECK(empty.finished());
    ScoreTracker t;
    CHECK_EQ(t.grade(), string("No answers yet"));
    t.record(true); t.record(true); t.record(false);
    CHECK_EQ(t.streak(), 0); CHECK_EQ(t.bestStreak(), 2);
    t.reset();
    CHECK_EQ(t.answered(), 0);
}

TEST_CASE(quiz_works_on_uniform_timelines_with_single_step_kind) {
    Timeline tl;
    for (int i = 0; i < 3; i++) { Step s; s.op = OpType::Compare; RoleTag r; r.target_id = i; r.role = Role::Compared; s.roles.push_back(r); tl.steps.push_back(s); }
    finalizeTimeline(tl);
    QuizController quiz(tl);
    while (!quiz.finished()) {
        QuizQuestion q = quiz.ask();
        CHECK(q.options.size() >= 2);
        quiz.answer(q, q.correct);
    }
    CHECK_EQ(quiz.score().correct(), 3);
}

TEST_CASE(report_is_valid_markdown_with_all_sections) {
    auto algo = builtin().find("bubble_sort")->make();
    Input in; in.values = {5, 1, 4, 2};
    Timeline tl = generate(*algo, in);
    ReportData rd; rd.info = &algo->info(); rd.timeline = &tl; rd.snapshot = "[1][2][4][5]\n";
    rd.comparison = compareTimelines({tl}, {"Bubble Sort"});
    string md = ReportExporter::render(rd);
    for (const char* heading : {"# Execution Summary - Bubble Sort", "## 1. Algorithm", "## 2. Input", "## 3. Execution", "## 4. Operation histogram",
                                "## 5. Theory versus measurement", "## 6. Comparison", "## Final picture", "## Step log"})
        CHECK(md.find(heading) != string::npos);
    CHECK(md.find("| Comparisons | 6 |") != string::npos);
    size_t pos = 0; int tables = 0;
    while ((pos = md.find("|---", pos)) != string::npos) { tables++; pos += 4; }
    CHECK(tables >= 4);
    CHECK_EQ(ReportExporter::escapeCell("a|b\nc"), string("a\\|b c"));
}

TEST_CASE(report_file_writing_and_errors) {
    auto algo = builtin().find("linear_search")->make();
    Input in; in.values = {1, 2, 3}; in.hasTarget = true; in.target = 3;
    Timeline tl = generate(*algo, in);
    ReportData rd; rd.info = &algo->info(); rd.timeline = &tl; rd.maxStepRows = 2;
    CHECK_EQ(ReportExporter::write("build/summary_test.md", rd), string(""));
    ifstream f("build/summary_test.md");
    string all((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(all.find("more step(s) not shown") != string::npos);
    CHECK(!ReportExporter::write("no_such_dir/x/summary.md", rd).empty());
    ReportData empty;
    CHECK(!ReportExporter::write("build/x.md", empty).empty());
}
