// Tests for src/plugin (M1): registry, adapters, malformed-step rejection, exception
// isolation, custom structures on every renderer primitive, and integration with
// Race / Quiz / Report (spec section 36, "Custom Algorithm / Custom Structure").
#include <algorithm>
#include <fstream>
#include <limits>

#include "../TestFramework.h"
#include "../../src/core/BuiltinCatalog.h"
#include "../../src/engine/quiz/QuizController.h"
#include "../../src/engine/race/RaceController.h"
#include "../../src/engine/report/ReportExporter.h"
#include "../../src/plugin/PluginLoader.h"
#include "../../src/plugin/examples/ExamplePlugins.h"
#include "../../src/render/RendererFactory.h"

// ------------------------------------------------------------- test plugins
struct TestPlugin : CustomAlgorithm {
    string id; int mode; StructureKind kind;
    TestPlugin(string i, int m, StructureKind k = StructureKind::Array) : id(i), mode(m), kind(k) {}
    PluginMetadata metadata() const override {
        PluginMetadata m; m.id = id; m.name = "Test " + id; m.kind = PluginKind::Algorithm; m.renders = kind; m.inputSchema = "array";
        return m;
    }
    void execute(const Input& in, PluginContext& ctx) override {
        switch (mode) {
        case 0: ctx.setArray(in.values); if (ctx.size() >= 2) ctx.compare(0, 1, "one step"); break;          // minimal valid
        case 1: break;                                                                                       // no-op: zero steps
        case 2: ctx.setArray(in.values); for (int i = 0; i < 20000; i++) ctx.note("tick"); break;           // stress
        case 3: ctx.emit(OpType::EdgeAdd, {Delta::edgeAdd(0, -5)}, {}, "bad edge"); break;                  // malformed delta
        case 4: ctx.emit(OpType::Note, {}, {}, "bad progress"); ctx.recorder().timeline().steps.back().progress = 7.f; break;
        case 5: throw runtime_error("plugin exploded");
        case 6: throw 42;
        case 7: ctx.compare(0, 1, "no setArray"); break;                                                     // misuse -> runtime_error
        case 8: ctx.emit(OpType::Highlight, {}, {RoleTag{-3, Role::Found, 0}}, "bad role"); break;
        }
    }
};

TEST_CASE(plugin_registry_accepts_valid_and_refuses_bad) {
    PluginRegistry reg;
    CHECK(reg.registerAlgorithm([] { return unique_ptr<CustomAlgorithm>(new TestPlugin("ok_one", 0)); }).empty());
    CHECK(!reg.registerAlgorithm([] { return unique_ptr<CustomAlgorithm>(new TestPlugin("ok_one", 0)); }).empty());     // duplicate id
    CHECK(!reg.registerAlgorithm([] { return unique_ptr<CustomAlgorithm>(new TestPlugin("Bad Id!", 0)); }).empty());    // invalid id
    CHECK(!reg.registerAlgorithm([]() -> unique_ptr<CustomAlgorithm> { throw runtime_error("ctor"); }).empty());        // constructor throws
    CHECK(!reg.registerAlgorithm([]() -> unique_ptr<CustomAlgorithm> { return nullptr; }).empty());
    CHECK_EQ(reg.count(), (size_t)1);
    CHECK(reg.log().size() >= 5);
    struct NoOps : CustomStructure {
        PluginMetadata metadata() const override { PluginMetadata m; m.id = "noops"; m.name = "n"; return m; }
        void apply(const Command&, PluginContext&) override {}
    };
    CHECK(!reg.registerStructure([] { return unique_ptr<CustomStructure>(new NoOps()); }).empty());       // structure without ops
}

static Timeline runPlugin(int mode, vector<int> values = {3, 1, 2}) {
    PluginRegistry reg;
    reg.registerAlgorithm([mode] { return unique_ptr<CustomAlgorithm>(new TestPlugin("t", mode)); });
    AlgorithmCatalog cat;
    reg.addToCatalog(cat);
    auto algo = cat.find("t")->make();
    Input in; in.values = values;
    return generate(*algo, in);
}

TEST_CASE(plugin_minimal_valid_noop_and_stress) {
    Timeline one = runPlugin(0);
    CHECK(one.error.empty());
    CHECK_EQ(one.steps.size(), (size_t)1);
    tf::verifyTimeline(one);
    Timeline none = runPlugin(1);
    CHECK(none.error.empty());
    CHECK_EQ(none.steps.size(), (size_t)0);
    tf::verifyTimeline(none);
    Timeline many = runPlugin(2);
    CHECK(many.error.empty());
    CHECK_EQ(many.steps.size(), (size_t)20000);
    ExecutionEngine e; e.load(many); e.toEnd(); e.jumpTo(12345);
    CHECK_EQ(e.position(), (size_t)12345);
}

TEST_CASE(plugin_malformed_steps_are_rejected_gracefully) {
    for (int mode : {3, 4, 8}) {
        Timeline tl = runPlugin(mode);
        CHECK(!tl.error.empty());
        CHECK(tl.error.find("malformed") != string::npos);
        CHECK(tl.steps.empty());
    }
}

TEST_CASE(plugin_exceptions_are_isolated) {
    for (int mode : {5, 6, 7}) {
        Timeline tl = runPlugin(mode);
        CHECK(!tl.error.empty());
        CHECK(tl.steps.empty());
    }
    CHECK(runPlugin(5).error.find("exploded") != string::npos);
    // the host still works afterwards
    CHECK(runPlugin(0).error.empty());
}

TEST_CASE(plugin_input_schema_validation) {
    Timeline tl = runPlugin(0, {});
    CHECK(!tl.error.empty());
}

TEST_CASE(plugin_validator_limits) {
    Timeline tl;
    CHECK(validateTimeline(tl).empty());
    Step s; s.deltas.push_back(Delta::value(0, 1, 99));
    tl.steps.push_back(s);
    CHECK(validateTimeline(tl).find("container") != string::npos);
    Timeline big;
    big.steps.resize(200001);
    CHECK(!validateTimeline(big).empty());
    Timeline nan;
    Step n; n.progress = numeric_limits<float>::quiet_NaN();
    nan.steps.push_back(n);
    CHECK(!validateTimeline(nan).empty());
    Timeline ok;
    Step g; g.progress = -1.f; g.deltas.push_back(Delta::pointer("i", 3));
    ok.steps.push_back(g);
    CHECK(validateTimeline(ok).empty());
}

TEST_CASE(plugin_example_algorithms_sort_correctly) {
    for (auto make : {+[]() -> unique_ptr<CustomAlgorithm> { return unique_ptr<CustomAlgorithm>(new GnomeSortPlugin()); },
                      +[]() -> unique_ptr<CustomAlgorithm> { return unique_ptr<CustomAlgorithm>(new CocktailSortPlugin()); }}) {
        for (const auto& arr : tf::sampleArrays()) {
            if (arr.empty()) continue;
            CustomAlgorithmAdapter adapter(make());
            Input in; in.values = arr;
            Timeline tl = generate(adapter, in);
            tf::verifyTimeline(tl);
            vector<int> expect = arr; sort(expect.begin(), expect.end());
            CHECK(tf::intsOf(tf::finalState(tl)) == expect);
        }
    }
}

TEST_CASE(plugin_custom_structure_deque_through_the_session_machinery) {
    PluginRegistry reg;
    registerExamplePlugins(reg);
    AlgorithmCatalog cat = buildBuiltinCatalog();
    reg.addToCatalog(cat);
    CHECK_EQ(cat.custom().size(), (size_t)2);
    const CatalogEntry* e = cat.find("deque");
    CHECK(e && e->input == InputKind::Structure && e->structure == StructureKind::Array);
    auto algo = e->make();
    Input in; in.values = {1, 2};
    Command a; a.op = "push_front"; a.a = 0; in.commands.push_back(a);
    Command b; b.op = "push_back"; b.a = 3; in.commands.push_back(b);
    Command c; c.op = "pop_front"; in.commands.push_back(c);
    in.visibleFrom = 1;
    Timeline tl = generate(*algo, in);
    CHECK(tl.error.empty());
    tf::verifyTimeline(tl);
    WorldState init; init.apply(tl.initial);
    CHECK(tf::intsOf(init) == (vector<int>{0, 1, 2}));
    CHECK(tf::intsOf(tf::finalState(tl)) == (vector<int>{1, 2, 3}));
    Input bad; Command p; p.op = "pop_back"; bad.commands.push_back(p);
    auto algo2 = e->make();
    CHECK(!generate(*algo2, bad).error.empty());               // underflow reported, not crashed
}

struct KindStructure : CustomStructure {
    StructureKind kind;
    explicit KindStructure(StructureKind k) : kind(k) {}
    PluginMetadata metadata() const override {
        PluginMetadata m; m.id = "kind_" + to_string((int)kind); m.name = "Kind"; m.kind = PluginKind::Structure; m.renders = kind;
        m.ops = {{"go", "Go", {}}};
        return m;
    }
    void apply(const Command&, PluginContext& ctx) override {
        // a tiny picture that every renderer family can draw: three nodes, two edges
        ctx.emit(OpType::NodeCreate, {Delta::create(0, 10), Delta::create(1, 20), Delta::create(2, 30), Delta::resize(3),
                 Delta::edgeAdd(0, 1, Slot::Left), Delta::edgeAdd(0, 2, Slot::Right), Delta::pointer("HEAD", 0), Delta::pointer("TOP", 2),
                 Delta::pointer("FRONT", 0), Delta::pointer("REAR", 2)}, {}, "built");
    }
};

TEST_CASE(plugin_custom_structure_maps_to_every_renderer_primitive) {
    for (StructureKind k : {StructureKind::Array, StructureKind::List, StructureKind::Tree, StructureKind::Graph, StructureKind::Hash,
                            StructureKind::Stack, StructureKind::Queue, StructureKind::Heap}) {
        CustomStructureAdapter adapter(unique_ptr<CustomStructure>(new KindStructure(k)));
        Input in; Command c; c.op = "go"; in.commands.push_back(c);
        Timeline tl = generate(adapter, in);
        CHECK(tl.error.empty());
        CHECK(tl.structure == k);
        ExecutionEngine e; e.load(tl); e.toEnd();
        Theme th = Theme::make(ThemeId::Mono);
        GlyphSet g = GlyphSet::ascii();
        RenderContext ctx(e.state(), e.currentStep(), tl, th, g);
        ctx.width = 80;
        Canvas canvas(80, 0);
        makeRenderer(k)->draw(canvas, ctx);
        CHECK(canvas.usedHeight() > 0);
        CHECK(!canvas.toPlain().empty());
    }
}

TEST_CASE(plugin_loader_reports_errors_and_loads_the_example_library) {
    PluginRegistry reg;
    CHECK(!PluginLoader::load("no/such/file.dll", reg).empty());
    ofstream("build/not_a_plugin.dll") << "this is text, not a library";
    CHECK(!PluginLoader::load("build/not_a_plugin.dll", reg).empty());
    ifstream lib("build/example_plugin.dll");
    ifstream lib2("build/example_plugin.so");
    if (lib || lib2) {
        string path = lib ? "build/example_plugin.dll" : "build/example_plugin.so";
        string err = PluginLoader::load(path, reg);
        CHECK_EQ(err, string(""));
        CHECK_EQ(reg.count(), (size_t)1);
        AlgorithmCatalog cat;
        reg.addToCatalog(cat);
        auto algo = cat.find("cocktail_sort")->make();
        Input in; in.values = {5, 2, 9, 1};
        Timeline tl = generate(*algo, in);
        CHECK(tl.error.empty());
        CHECK(tf::intsOf(tf::finalState(tl)) == (vector<int>{1, 2, 5, 9}));
    } else {
        cerr << "  (skipped: build/example_plugin.dll not built - run `make plugin-dll`)\n";
    }
}

TEST_CASE(plugin_custom_algorithm_works_in_race_quiz_and_report) {
    PluginRegistry reg;
    registerExamplePlugins(reg);
    AlgorithmCatalog cat = buildBuiltinCatalog();
    reg.addToCatalog(cat);
    Input in; in.values = {8, 3, 5, 1, 9, 2};
    auto builtin = cat.find("bubble_sort")->make();
    auto custom = cat.find("gnome_sort")->make();
    Timeline ta = generate(*builtin, in), tb = generate(*custom, in);

    RaceController race;
    race.load(ta, tb, "Bubble Sort", "Gnome Sort (plugin)");
    while (race.tick()) {}
    CHECK(race.finished());
    CHECK(tf::intsOf(race.paneA().state()) == tf::intsOf(race.paneB().state()));

    QuizController quiz(tb);
    int asked = 0;
    while (!quiz.finished()) { QuizQuestion q = quiz.ask(); quiz.answer(q, q.correct); asked++; }
    CHECK(asked > 0);
    CHECK_EQ(quiz.score().correct(), asked);

    ReportData rd; rd.info = &custom->info(); rd.timeline = &tb;
    string md = ReportExporter::render(rd);
    CHECK(md.find("Gnome Sort (plugin)") != string::npos);
    CHECK(md.find("| SWAP |") != string::npos);
}
