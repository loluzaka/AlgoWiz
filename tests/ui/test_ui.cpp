// Tests for src/ui (M2): input validation, settings persistence, panels, key handling of the
// visualizer screen, headless commands (--list / --dump).
#include <fstream>
#include <set>

#include "../TestFramework.h"
#include "../../src/core/BuiltinCatalog.h"
#include "../../src/ui/Headless.h"
#include "../../src/ui/InputManager.h"
#include "../../src/ui/Settings.h"
#include "../../src/ui/UIManager.h"
#include "../../src/ui/VisualizerScreen.h"
#include "../../src/ui/quiz/QuizScreen.h"
#include "../../src/ui/race/RaceScreen.h"

using namespace InputManager;

TEST_CASE(ui_parse_int_accepts_and_rejects) {
    int v = 0;
    CHECK(parseInt("42", 0, 100, v).empty() && v == 42);
    CHECK(parseInt("  -7 ", -10, 10, v).empty() && v == -7);
    CHECK(parseInt("+5", 0, 10, v).empty() && v == 5);
    CHECK(!parseInt("", 0, 10, v).empty());
    CHECK(!parseInt("   ", 0, 10, v).empty());
    CHECK(!parseInt("abc", 0, 10, v).empty());
    CHECK(!parseInt("4x", 0, 10, v).empty());
    CHECK(!parseInt("3.5", 0, 10, v).empty());
    CHECK(!parseInt("-", 0, 10, v).empty());
    CHECK(!parseInt("11", 0, 10, v).empty());
    CHECK(!parseInt("99999999999999", 0, 10, v).empty());
    CHECK(parseInt("11", 0, 10, v).find("between 0 and 10") != string::npos);   // understandable message
}

TEST_CASE(ui_parse_lists_and_words) {
    vector<int> v;
    CHECK(parseIntList("5 3, 8;1", 1, 10, -99, 99, v).empty());
    CHECK(v == (vector<int>{5, 3, 8, 1}));
    CHECK(!parseIntList("", 1, 5, 0, 9, v).empty());
    CHECK(!parseIntList("1 2 3", 1, 2, 0, 9, v).empty());
    CHECK(!parseIntList("1 x 3", 1, 5, 0, 9, v).empty());
    CHECK(!parseIntList("1 200", 1, 5, 0, 9, v).empty());
    vector<string> w;
    CHECK(parseWords("pear, apple", 1, 5, 14, w).empty());
    CHECK(w == (vector<string>{"pear", "apple"}));
    CHECK(!parseWords("", 1, 5, 14, w).empty());
    CHECK(!parseWords("a b c", 1, 2, 14, w).empty());
    CHECK(!parseWords("supercalifragilistic", 1, 2, 14, w).empty());
    CHECK_EQ(randomValues(5, 3, 3).size(), (size_t)5);
    for (int x : randomValues(50, -4, 4, 7)) CHECK(x >= -4 && x <= 4);
    CHECK_EQ(trim("  a b \n"), string("a b"));
}

TEST_CASE(ui_settings_round_trip_and_bad_files) {
    Settings s;
    s.theme = ThemeId::ColorBlind; s.speed = SpeedLevel::Fast; s.unicode = true; s.showCode = true;
    CHECK(s.save("build/test_settings.ini"));
    Settings t;
    CHECK(t.load("build/test_settings.ini"));
    CHECK(t.theme == ThemeId::ColorBlind && t.speed == SpeedLevel::Fast && t.unicode && t.showCode);
    ofstream("build/bad_settings.ini") << "theme=99\nspeed=-3\nnonsense\nunicode=\n=1\n";
    Settings u;
    CHECK(u.load("build/bad_settings.ini"));
    CHECK(u.theme == ThemeId::Dark && u.speed == SpeedLevel::Normal);          // invalid values are ignored
    Settings none;
    CHECK(!none.load("build/does_not_exist.ini"));
}

TEST_CASE(ui_wrap_text_and_progress_bar) {
    auto l = wrapText("the quick brown fox jumps", 10);
    CHECK(l == (vector<string>{"the quick", "brown fox", "jumps"}));
    auto longWord = wrapText("abcdefghijklmnop", 6);
    CHECK(longWord.size() == 3 && longWord[0] == "abcdef");
    CHECK(wrapText("x", 2).empty());                                             // unusable width: no crash
    CHECK_EQ(progressBar(0.5, 10), string("[#####-----]"));
    CHECK_EQ(progressBar(0.0, 4), string("[----]"));
    CHECK_EQ(progressBar(1.0, 4), string("[####]"));
}

static VisualizerScreen* makeScreen(Settings& st, AlgorithmCatalog& cat, const string& id, Timeline* keep = nullptr) {
    const CatalogEntry* e = cat.find(id);
    static unique_ptr<Algorithm> algo;
    algo = e->make();
    Timeline tl = generate(*algo, Headless::defaultInput(*e));
    if (keep) *keep = tl;
    VisualizerScreen* s = new VisualizerScreen(st, cat, *e, algo->info());
    s->load(tl);
    return s;
}

TEST_CASE(ui_visualizer_keys_control_playback) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Settings st;
    unique_ptr<VisualizerScreen> s(makeScreen(st, cat, "bubble_sort"));
    ExecutionEngine& e = s->engine();
    CHECK_EQ(s->handleKey('n'), 0); CHECK_EQ(e.position(), (size_t)1);
    s->handleKey(K_RIGHT); CHECK_EQ(e.position(), (size_t)2);
    s->handleKey('b'); CHECK_EQ(e.position(), (size_t)1);
    s->handleKey(K_LEFT); CHECK_EQ(e.position(), (size_t)0);
    s->handleKey('b'); CHECK_EQ(e.position(), (size_t)0);                          // cannot go before the start
    s->handleKey('p'); CHECK(s->playback().playing);
    s->handleKey(K_SPACE); CHECK(!s->playback().playing);
    s->handleKey(K_SPACE); CHECK(s->playback().playing);
    s->handleKey('n'); CHECK(!s->playback().playing);                              // stepping pauses
    s->handleKey(K_END); CHECK(e.atEnd());
    s->handleKey('p'); CHECK(e.atStart() && s->playback().playing);                // play at the end restarts
    s->handleKey('r'); CHECK(e.atStart() && !s->playback().playing);
    SpeedLevel before = s->playback().level;
    s->handleKey('s'); CHECK(s->playback().level != before);
    s->handleKey('f'); CHECK(s->playback().fastForward);
    s->handleKey('f'); CHECK(!s->playback().fastForward);
    s->handleKey('k'); CHECK(s->playback().skipAnimation && s->playback().playing);
    s->handleKey('r');
    string controls = s->compose(140, 35).toPlain();
    for (const char* removed : {"[J] Jump", "[F] Fast", "[K] Skip", "[V] View", "[T] Theme", "[E] Export", "[H] Help"})
        CHECK(controls.find(removed) == string::npos);
    CHECK_EQ(s->handleKey('i'), 2);                                                // new input
    CHECK_EQ(s->handleKey('q'), 1);
    CHECK_EQ(s->handleKey(K_ESC), 1);
}

TEST_CASE(ui_code_toggle_preserves_playback_and_theme_hotkey_remains_available) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Settings st;
    unique_ptr<VisualizerScreen> s(makeScreen(st, cat, "quick_sort"));
    s->handleKey('n'); s->handleKey('n'); s->handleKey('p');
    size_t pos = s->engine().position();
    bool playing = s->playback().playing;
    bool code = st.showCode;
    s->handleKey('c'); s->handleKey('t');
    CHECK_EQ(s->engine().position(), pos);
    CHECK_EQ(s->playback().playing, playing);
    CHECK_EQ(st.showCode, !code);
    CHECK(st.theme == Theme::next(ThemeId::Dark));
}

TEST_CASE(ui_visualizer_layout_has_no_left_algorithm_panel) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Settings st;
    unique_ptr<VisualizerScreen> s(makeScreen(st, cat, "bubble_sort"));
    string plain = s->compose(140, 35).toPlain();
    CHECK(plain.find("ALGORITHMS") == string::npos);
    CHECK(plain.find("INFORMATION") != string::npos);
    CHECK(plain.find("Array") != string::npos);
}

TEST_CASE(ui_code_panel_highlights_the_current_line) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Settings st; st.showCode = true; st.theme = ThemeId::Dark;
    unique_ptr<VisualizerScreen> s(makeScreen(st, cat, "bubble_sort"));
    s->engine().jumpTo(2);
    int line = s->engine().currentStep()->codeLine;
    CHECK(line >= 0);
    Canvas f = s->compose(120, 30);
    bool inverse = false;
    for (int y = 0; y < f.height(); y++) for (int x = 0; x < f.width(); x++) if (f.at(x, y).st.inverse) inverse = true;
    CHECK(inverse);
    set<int> done = executedLines(s->engine().timeline(), 5);
    CHECK(!done.empty());
}

TEST_CASE(ui_code_toggle_shows_cpp_source_for_sorting_algorithms) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    int checked = 0;
    for (const CatalogEntry* entry : cat.inCategory("Sorting")) {
        unique_ptr<Algorithm> algo = entry->make();
        Input input = Headless::defaultInput(*entry);
        Timeline timeline = generate(*algo, input);
        CHECK(timeline.error.empty());
        bool hasMappedStep = false;
        size_t mappedPosition = 0;
        for (size_t i = 0; i < timeline.steps.size(); i++) {
            const Step& step = timeline.steps[i];
            auto line = algo->info().cppSourceBlocks.find(step.codeLine);
            if (line != algo->info().cppSourceBlocks.end() && line->second.find(step.op) != line->second.end()) {
                hasMappedStep = true;
                mappedPosition = i + 1;
                break;
            }
        }
        CHECK(hasMappedStep);
        if (!hasMappedStep) continue;

        Settings settings;
        settings.theme = ThemeId::Mono;
        VisualizerScreen screen(settings, cat, *entry, algo->info());
        screen.load(std::move(timeline));
        CHECK_EQ(screen.handleKey('c'), 0);
        CHECK(settings.showCode);
        screen.engine().jumpTo(mappedPosition);
        string rendered = screen.compose(120, 35).toPlain();
        CHECK(rendered.find("C++ SOURCE:") != string::npos);
        CHECK(rendered.find("(C++ source block unavailable)") == string::npos);
        checked++;
    }
    CHECK_EQ((size_t)checked, cat.inCategory("Sorting").size());
}

TEST_CASE(ui_code_view_keeps_complexities_and_scopes_tree_pseudocode) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    auto sourceIsHighlighted = [](const Canvas& frame, const string& text) {
        string rendered = frame.toPlain();
        size_t pos = rendered.find(text);
        if (pos == string::npos) return false;
        int row = (int)count(rendered.begin(), rendered.begin() + pos, '\n');
        size_t rowStart = rendered.rfind('\n', pos);
        int col = (int)(pos - (rowStart == string::npos ? 0 : rowStart + 1));
        return frame.at(col, row).st.inverse;
    };
    for (const string id : {"binary_tree", "bst", "avl"}) {
        const CatalogEntry* e = cat.find(id);
        auto algo = e->make();
        Input in = Headless::defaultInput(*e);
        Command minCommand; minCommand.op = "min";
        in.commands.push_back(minCommand);
        Timeline tl = generate(*algo, in);
        Settings st; st.showCode = true; st.theme = ThemeId::Dark;
        VisualizerScreen screen(st, cat, *e, algo->info());
        screen.load(tl);
        screen.engine().next();

        CHECK_EQ(screen.engine().currentStep()->operation, string("min"));
        Canvas frame = screen.compose(120, 35);
        string plain = frame.toPlain();
        CHECK(plain.find("TIME COMPLEXITY") != string::npos);
        CHECK(plain.find("SPACE COMPLEXITY") != string::npos);
        CHECK(plain.find("LIVE EXECUTION") != string::npos);
        CHECK(plain.find("Best:") != string::npos);
        CHECK(plain.find("Average:") != string::npos);
        CHECK(plain.find("Worst:") != string::npos);
        CHECK(plain.find("Space:") != string::npos);
        CHECK(plain.find("OPERATION PSEUDOCODE: MIN") != string::npos);
        CHECK(plain.find("min:") != string::npos);
        CHECK(plain.find("max:") == string::npos);
        CHECK(plain.find("C++ SOURCE:") != string::npos);
        string expected = id == "binary_tree" ? "min: BFS and keep the smallest key" : "min: follow left children";
        CHECK(plain.find(expected) != string::npos);
        string expectedSource = id == "binary_tree" ? "bool better = wantMax" : "TNode* nxt = wantMax";
        size_t sourcePos = plain.find(expectedSource);
        CHECK(sourcePos != string::npos);
        CHECK(sourceIsHighlighted(frame, expectedSource));

        while (!screen.engine().atEnd() && screen.engine().currentStep()->op != OpType::Highlight)
            screen.engine().next();
        CHECK(screen.engine().currentStep() != nullptr);
        CHECK(screen.engine().currentStep()->op == OpType::Highlight);
        Canvas finalFrame = screen.compose(120, 35);
        string finalSource = id == "binary_tree" ? "ev(OpType::Highlight," : "if (!nxt) break;";
        CHECK(sourceIsHighlighted(finalFrame, finalSource));
    }
}

TEST_CASE(ui_error_timelines_are_shown_not_crashed) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("binary_search");
    auto algo = e->make();
    Input in; in.values = {3, 1, 2}; in.hasTarget = true; in.target = 1;
    Settings st; st.theme = ThemeId::Mono;
    VisualizerScreen s(st, cat, *e, algo->info());
    s.load(generate(*algo, in));
    string plain = s.compose(110, 30).toPlain();
    CHECK(plain.find("cannot be run") != string::npos);
    CHECK(plain.find("sorted") != string::npos);
}

TEST_CASE(ui_headless_list_and_dump) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    string list = Headless::list(cat);
    for (const auto& e : cat.all()) CHECK(list.find(e.id) != string::npos);
    bool ok = false;
    string out = Headless::dump(cat, {"bubble_sort", "--values", "3,1,2", "--step", "2"}, ok);
    CHECK(ok && out.find("BUBBLE SORT") != string::npos && out.find("STEP 2 /") != string::npos);
    string steps = Headless::dump(cat, {"bst", "--ops", "insert 5; insert 3", "--steps"}, ok);
    CHECK(ok && steps.find("NODE_CREATE") != string::npos);
    string rep = Headless::dump(cat, {"selection_sort", "--report"}, ok);
    CHECK(ok && rep.find("# Execution Summary - Selection Sort") != string::npos);
    Headless::dump(cat, {"nope"}, ok); CHECK(!ok);
    Headless::dump(cat, {}, ok); CHECK(!ok);
    Headless::dump(cat, {"bubble_sort", "--bogus"}, ok); CHECK(!ok);
    string bad = Headless::dump(cat, {"binary_search", "--values", "3,1,2", "--target", "1"}, ok);
    CHECK(ok && bad.find("cannot be run") != string::npos);
    CHECK_EQ(Headless::splitBy("a,b,,c", ',').size(), (size_t)4);
    CHECK(Headless::parseCsv("1,-2,x") == (vector<int>{1, -2, 0}));
    CHECK_EQ(Headless::parseOps("insert 5; delete 3 ; ;add_edge 0 1 4").size(), (size_t)3);
}

TEST_CASE(ui_menu_categories_match_the_home_menu) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    CHECK_EQ(menuCategories().size(), (size_t)10);
    for (const string& c : menuCategories()) CHECK(!cat.inCategory(c).empty());
    CHECK_EQ(homeMenuItems().size(), (size_t)11);
    CHECK(homeMenuItems()[10].find("Race Mode") != string::npos);
    for (const string& item : homeMenuItems()) {
        CHECK(item.find("Quiz") == string::npos);
        CHECK(item.find("Custom") == string::npos);
    }
    set<string> ids;
    for (const auto& e : cat.all()) { CHECK(ids.insert(e.id).second); CHECK(static_cast<bool>(e.make)); }
}

TEST_CASE(ui_race_screen_shows_both_panes_and_the_final_table) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Input in; in.values = {8, 3, 5, 1, 9, 2};
    auto a = cat.find("bubble_sort")->make(), b = cat.find("merge_sort")->make();
    RaceController race;
    race.load(generate(*a, in), generate(*b, in), "Bubble Sort", "Merge Sort");
    Settings st; st.theme = ThemeId::Mono;
    RaceScreen screen(st, race, in, a->info(), b->info());
    race.tick(); race.tick();
    string plain = screen.compose(110, 30).toPlain();
    CHECK(plain.find("RACE MODE") != string::npos && plain.find("Bubble Sort") != string::npos && plain.find("Merge Sort") != string::npos);
    CHECK(plain.find("TICK 2") != string::npos);
    CHECK_EQ(screen.handleKey('m'), 0);                       // switches to "same fraction done"
    CHECK(race.mode() == RaceMode::Progress);
    CHECK_EQ(screen.handleKey('r'), 0);
    while (race.tick()) {}
    string done = screen.compose(110, 30).toPlain();
    CHECK(done.find("Time B/A/W:") != string::npos);
    CHECK(done.find("Space:") != string::npos);
    CHECK(done.find("Bubble Sort") != string::npos && done.find("Merge Sort") != string::npos);
    CHECK(done.find("Assignments:") != string::npos);
    CHECK(done.find("Recorded:") != string::npos);
    CHECK(done.find("Fewer steps") != string::npos);
    CHECK_EQ(screen.handleKey('q'), 1);
    CHECK(screen.compose(60, 15).toPlain().find("needs a terminal") != string::npos);
}

TEST_CASE(ui_quiz_screen_shows_question_and_result) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    Input in; in.values = {3, 1, 2};
    auto a = cat.find("bubble_sort")->make();
    QuizController quiz(generate(*a, in), 1);
    ScoreTracker session;
    Settings st; st.theme = ThemeId::Mono;
    QuizScreen screen(st, quiz, "Bubble Sort", session);
    QuizQuestion q = quiz.ask();
    string asking = screen.compose(100, 32, &q, -1, false).toPlain();
    CHECK(asking.find("WHAT HAPPENS NEXT?") != string::npos && asking.find("A) ") != string::npos && asking.find("D) ") != string::npos);
    string right = screen.compose(100, 32, &q, q.correct, true).toPlain();
    CHECK(right.find("Correct!") != string::npos);
    string wrong = screen.compose(100, 32, &q, (q.correct + 1) % (int)q.options.size(), true).toPlain();
    CHECK(wrong.find("Not quite.") != string::npos);
    session.record(true); session.record(false);
    CHECK(screen.summary(80, 20).toPlain().find("1 / 2") != string::npos || screen.summary(80, 20).toPlain().find("QUIZ RESULT") != string::npos);
}
