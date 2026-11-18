// Tests for src/render (M2 for theme / primitives, M1/M3/M4 for their renderers): Canvas,
// themes, glyph sets and every renderer in colour / monochrome, Unicode / ASCII and at
// narrow terminal widths (spec section 36 "Rendering should also be tested with ...").
#include <algorithm>
#include <map>

#include "../TestFramework.h"
#include "../../src/core/BuiltinCatalog.h"
#include "../../src/plugin/examples/ExamplePlugins.h"
#include "../../src/render/RendererFactory.h"
#include "../../src/ui/Headless.h"
#include "../../src/ui/VisualizerScreen.h"

// One short operation script per structure so every picture is exercised.
static const map<string, string>& opScripts() {
    static const map<string, string> m = {
        {"array", "insert 1 99; delete 0; update 0 5; access 0; traverse"},
        {"singly_list", "insert_end 5; insert_begin 1; insert_pos 1 9; search 9; delete_end; delete_pos 1; delete_begin; traverse"},
        {"doubly_list", "insert_end 5; insert_begin 1; insert_pos 1 9; search 9; delete_end; delete_pos 1; delete_begin; traverse"},
        {"circular_list", "insert_end 5; insert_begin 1; insert_pos 1 9; search 9; delete_end; delete_pos 1; delete_begin; traverse"},
        {"stack", "push 1; push 2; pop; peek; is_empty; is_full"},
        {"queue", "enqueue 1; enqueue 2; dequeue; front; rear; is_empty; is_full"},
        {"circular_queue", "enqueue 1; enqueue 2; dequeue; front; rear; is_empty; is_full"},
        {"hash_linear", "insert 15; insert 22; search 22; delete 15; search 15"},
        {"hash_quadratic", "insert 15; insert 22; search 22; delete 15; search 15"},
        {"hash_chaining", "insert 15; insert 22; search 22; delete 15; search 15"},
        {"binary_tree", "insert 5; insert 3; insert 8; search 3; min; max; inorder; preorder; postorder; delete 5"},
        {"bst", "insert 5; insert 3; insert 8; search 3; min; max; inorder; preorder; postorder; delete 5"},
        {"avl", "insert 5; insert 3; insert 8; insert 1; insert 0; search 3; min; max; inorder; delete 5"},
        {"min_heap", "insert 4; insert 1; extract; peek"}, {"max_heap", "insert 4; insert 1; extract; peek"},
        {"priority_queue", "insert 4; insert 1; extract; peek"},
        {"graph", "add_vertex; add_edge 4 5 3; set_weight 4 5 4; bfs 0; dfs 0; delete_edge 4 5; delete_vertex 1"},
        {"deque", "push_back 1; push_front 2; pop_back; pop_front"}};
    return m;
}

static Input inputFor(const CatalogEntry& e) {
    Input in = Headless::defaultInput(e);
    if (e.input == InputKind::Structure) {
        auto it = opScripts().find(e.id);
        if (it != opScripts().end()) { in.commands = Headless::parseOps(it->second); in.visibleFrom = 0; }
    }
    return in;
}

TEST_CASE(canvas_put_clip_utf8_and_plain_output) {
    Canvas c(5, 2);
    c.put(3, 0, "abcdef");                          // clipped at the right edge
    CHECK_EQ(c.toPlain(1), string("   ab\n"));
    c.put(0, 1, "\xE2\x94\x80\xE2\x94\x82");        // two multi-byte characters occupy two cells
    CHECK_EQ(textWidth("\xE2\x94\x80\xE2\x94\x82"), 2);
    CHECK_EQ(c.at(1, 1).ch, string("\xE2\x94\x82"));
    c.put(0, 5, "grow");                            // the canvas grows downwards
    CHECK_EQ(c.height(), 6);
    c.put(-2, 0, "xyz");                            // negative x is clipped, not a crash
    CHECK_EQ(c.at(0, 0).ch, string("z"));
    c.putCentered(0, 5, 3, "ab");
    CHECK_EQ(c.at(1, 3).ch, string("a"));
}

TEST_CASE(canvas_ansi_output_only_when_styles_change) {
    Canvas c(6, 1);
    Style red; red.fg = 9; Style bold; bold.bold = true; bold.bg = 17;
    c.put(0, 0, "ab", red); c.put(2, 0, "cd", red); c.put(4, 0, "e", bold);
    string a = c.toAnsi();
    CHECK(a.find("\x1b[0;91m") != string::npos);            // bright red foreground
    CHECK(a.find("\x1b[0;1;48;5;17m") != string::npos);     // bold + 256-colour background
    size_t first = a.find("\x1b[0;91m");
    CHECK(a.find("\x1b[0;91m", first + 1) == string::npos); // not repeated for "cd"
    CHECK(a.find("\x1b[0m") != string::npos);
    CHECK(c.toPlain().find("\x1b") == string::npos);
    CHECK(Canvas(3, 1).toAnsi().find("\x1b") == string::npos);   // unstyled canvas: no escape codes at all
}

TEST_CASE(canvas_blit_line_fill_and_used_height) {
    Canvas src(4, 2); src.put(0, 0, "ab"); src.put(0, 1, "cd");
    Canvas dst(6, 3);
    dst.blit(src, 1, 0, 3, 2, 2, 1);                // copies "b", "d" (clipped on the right)
    CHECK_EQ(dst.at(2, 1).ch, string("b"));
    CHECK_EQ(dst.at(2, 2).ch, string("d"));
    CHECK_EQ(dst.usedHeight(), 3);
    Canvas l(10, 6);
    l.line(0, 0, 0, 4, Style());
    CHECK_EQ(l.at(0, 2).ch, string("|"));
    Canvas h(10, 6);
    h.line(0, 3, 8, 3, Style());
    CHECK_EQ(h.at(4, 3).ch, string("-"));
    Canvas f(4, 2); f.fill(0, 0, 4, 2, "#");
    CHECK_EQ(f.toPlain(), string("####\n####\n"));
    CHECK_EQ(Canvas(4, 4).usedHeight(), 0);
}

TEST_CASE(themes_define_every_role_and_respect_fallback_rules) {
    for (int t = 0; t < 5; t++) {
        Theme th = Theme::make(static_cast<ThemeId>(t));
        for (int r = 1; r <= static_cast<int>(Role::GroupB); r++) {
            Theme badged = th; badged.badges = true;
            CHECK(!badged.badge(static_cast<Role>(r)).empty());                 // every role has a text tag
        }
        CHECK(th.role(Role::Default).badge.empty());
    }
    Theme mono = Theme::make(ThemeId::Mono);
    CHECK(!mono.color && mono.badges);
    for (int r = 0; r <= static_cast<int>(Role::GroupB); r++) CHECK(mono.role(static_cast<Role>(r)).style.plain());   // mono: no styling at all
    Theme dark = Theme::make(ThemeId::Dark);
    CHECK(dark.color && !dark.badges);
    CHECK(Theme::make(ThemeId::HighContrast).badges && Theme::make(ThemeId::ColorBlind).badges);
    // roles must stay distinguishable in each coloured theme
    for (ThemeId id : {ThemeId::Dark, ThemeId::Light, ThemeId::HighContrast, ThemeId::ColorBlind}) {
        Theme th = Theme::make(id);
        CHECK(th.role(Role::Compared).style != th.role(Role::Moved).style);
        CHECK(th.role(Role::Sorted).style != th.role(Role::Moved).style);
        CHECK(th.role(Role::Compared).style != th.role(Role::Sorted).style);
        CHECK(th.role(Role::Pivot).style != th.role(Role::Compared).style);
        CHECK(th.role(Role::Found).style != th.role(Role::NotFound).style);
    }
    CHECK(Theme::next(ThemeId::Mono) == ThemeId::Dark);
    CHECK(Theme::name(ThemeId::ColorBlind).find("Color") != string::npos);
}

TEST_CASE(glyph_sets_ascii_is_pure_ascii) {
    GlyphSet a = GlyphSet::ascii(), u = GlyphSet::utf8();
    for (const string* s : {&a.h, &a.v, &a.tl, &a.tr, &a.bl, &a.br, &a.arrowR, &a.arrowL, &a.arrowBoth, &a.arrowUp, &a.arrowDown, &a.dot, &a.right, &a.left})
        for (unsigned char ch : *s) CHECK(ch < 128);
    CHECK(u.unicode && u.h.size() > 1);
    CHECK_EQ(a.repeat("ab", 3), string("ababab"));
}

TEST_CASE(every_algorithm_renders_in_every_theme_width_and_glyph_set) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    PluginRegistry reg;
    registerExamplePlugins(reg);
    reg.addToCatalog(cat);
    int frames = 0;
    for (const CatalogEntry& e : cat.all()) {
        Input in = inputFor(e);
        auto algo = e.make();
        Timeline tl = generate(*algo, in);
        if (!tl.error.empty()) cerr << "  " << e.id << ": " << tl.error << '\n';
        CHECK(tl.error.empty());
        for (int theme = 0; theme < 5; theme++) {
            for (int width : {60, 80, 120, 160}) {
                for (bool uni : {false, true}) {
                    Settings st; st.theme = static_cast<ThemeId>(theme); st.unicode = uni; st.showCode = (width == 120);
                    VisualizerScreen screen(st, cat, e, algo->info());
                    screen.load(tl);
                    for (size_t pos : {(size_t)0, tl.steps.size() / 2, tl.steps.size()}) {
                        screen.engine().jumpTo(pos);
                        Canvas f = screen.compose(width, 30);
                        CHECK_EQ(f.width(), width);
                        string plain = f.toPlain();
                        CHECK(plain.find("ALGO VISUALIZER") != string::npos);
                        string ansi = f.toAnsi();
                        if (theme == 4) CHECK(!Theme::make(ThemeId::Mono).color);
                        if (!uni) for (unsigned char ch : plain) if (ch >= 128) { CHECK(false); break; }     // ASCII fallback must be pure ASCII
                        frames++;
                    }
                }
            }
        }
    }
    CHECK(frames > 3000);
}

TEST_CASE(small_terminal_shows_a_graceful_message) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("bubble_sort");
    auto algo = e->make();
    Settings st;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(generate(*algo, Headless::defaultInput(*e)));
    Canvas f = screen.compose(40, 10);
    CHECK(f.toPlain().find("Terminal too small") != string::npos);
}

TEST_CASE(monochrome_shows_text_badges_and_colour_themes_do_not) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("bubble_sort");
    auto algo = e->make();
    Timeline tl = generate(*algo, Headless::defaultInput(*e));
    size_t cmp = 0;
    for (size_t i = 0; i < tl.steps.size(); i++) if (tl.steps[i].op == OpType::Compare) { cmp = i + 1; break; }
    for (int theme : {0, 4}) {
        Settings st; st.theme = static_cast<ThemeId>(theme);
        VisualizerScreen screen(st, cat, *e, algo->info());
        screen.load(tl);
        screen.engine().jumpTo(cmp);
        string plain = screen.compose(110, 30).toPlain();
        CHECK_EQ(plain.find("[CMP]") != string::npos, theme == 4);
    }
}

TEST_CASE(tree_picture_contains_all_nodes_and_ascii_edges) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("bst");
    auto algo = e->make();
    Input in = Headless::defaultInput(*e);
    Timeline tl = generate(*algo, in);
    Settings st; st.theme = ThemeId::Mono;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(tl);
    string plain = screen.compose(120, 30).toPlain();
    for (const char* k : {"(50)", "(30)", "(70)", "(20)", "(40)", "(60)", "(80)"}) CHECK(plain.find(k) != string::npos);
    CHECK(plain.find("/") != string::npos && plain.find("\\") != string::npos);
}

TEST_CASE(wide_tree_scrolls_horizontally) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("avl");
    auto algo = e->make();
    Input in;
    for (int i = 1; i <= 31; i++) in.values.push_back(i);
    Timeline tl = generate(*algo, in);
    Settings st; st.theme = ThemeId::Mono;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(tl);
    string before = screen.compose(100, 40).toPlain();
    CHECK(before.find("scroll with [") != string::npos);
    screen.handleKey(']'); screen.handleKey(']'); screen.handleKey(']');
    string after = screen.compose(100, 40).toPlain();
    CHECK(before != after);
}

TEST_CASE(graph_has_three_views) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("graph");
    auto algo = e->make();
    Input in; in.vertexCount = 3; in.directed = true; in.values = {0, 1, -7, 1, 2, 0};
    Timeline tl = generate(*algo, in);
    Settings st; st.theme = ThemeId::Mono;
    st.unicode = false;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(tl);
    string matrix = screen.compose(110, 30).toPlain();
    CHECK(matrix.find("view: adjacency matrix") != string::npos);
    CHECK(matrix.find("Adjacency matrix (rows: from, columns: to; . = no edge)") != string::npos);
    CHECK(matrix.find("|  A   |  .  | -7  |  .  |") != string::npos);
    CHECK(matrix.find("|  B   |  .  |  .  |  0  |") != string::npos);
    screen.handleKey('v');
    CHECK(screen.compose(110, 30).toPlain().find("view: drawing") != string::npos);
    screen.handleKey('v');
    string list = screen.compose(110, 30).toPlain();
    CHECK(list.find("view: adjacency list") != string::npos);
    CHECK(list.find("B(-7)") != string::npos);

    Input wide; wide.vertexCount = 10; wide.values = {0, 1, 123456};
    auto wideAlgo = e->make();
    VisualizerScreen wideScreen(st, cat, *e, wideAlgo->info());
    wideScreen.load(generate(*wideAlgo, wide));
    string left = wideScreen.compose(60, 40).toPlain();
    CHECK(left.find("scroll with [ and ]") != string::npos);
    wideScreen.handleKey(']');
    CHECK(left != wideScreen.compose(60, 40).toPlain());
}

TEST_CASE(array_pointer_labels_and_wrapping) {
    AlgorithmCatalog cat = buildBuiltinCatalog();
    const CatalogEntry* e = cat.find("binary_search");
    auto algo = e->make();
    Input in; in.values = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23}; in.hasTarget = true; in.target = 17;
    Timeline tl = generate(*algo, in);
    Settings st; st.theme = ThemeId::Mono;
    VisualizerScreen screen(st, cat, *e, algo->info());
    screen.load(tl);
    size_t rangeStep = 0;
    for (size_t i = 0; i < tl.steps.size() && !rangeStep; i++)
        for (const auto& r : tl.steps[i].roles) if (r.role == Role::Range) rangeStep = i + 1;
    CHECK(rangeStep > 0);
    screen.engine().jumpTo(rangeStep);
    CHECK(screen.compose(60, 30).toPlain().find("[RNG]") != string::npos);
    size_t cmpStep = 0;
    for (size_t i = 0; i < tl.steps.size() && !cmpStep; i++) if (tl.steps[i].op == OpType::Compare) cmpStep = i + 1;
    screen.engine().jumpTo(cmpStep);
    string plain = screen.compose(60, 30).toPlain();
    CHECK(plain.find("^mid") != string::npos || plain.find("^lo,mid") != string::npos || plain.find("mid") != string::npos);
}
