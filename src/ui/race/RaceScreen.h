#ifndef RACESCREEN_H
#define RACESCREEN_H

// Race Mode screen: two panes side by side driven by one RaceController, per-pane statistics
// and a progress bar each, and the comparison table at the end.

#include <chrono>
#include <string>

#include "../../engine/Playback.h"
#include "../../engine/race/RaceController.h"
#include "../../engine/report/ReportExporter.h"
#include "../Settings.h"
#include "../Terminal.h"
#include "../VisualizerScreen.h"
#include "../panels/ExplanationBar.h"
#include "../panels/InfoPanel.h"

using namespace std;

class RaceScreen {
public:
    RaceScreen(Settings& s, RaceController& race, const Input& in, const AlgorithmInfo& infoA, const AlgorithmInfo& infoB)
        : settings_(s), race_(race), input_(in), infoA_(infoA), infoB_(infoB) {}

    Canvas compose(int W, int H) {
        Theme th = Theme::make(settings_.theme);
        GlyphSet g = glyphsFor(settings_);
        Canvas f(W, H);
        if (W < 78 || H < 20) {
            f.put(0, 0, "Race Mode needs a terminal of at least 79 x 21 (now " + to_string(W) + " x " + to_string(H) + ").", th.error);
            return f;
        }
        string title = "ALGO VISUALIZER | RACE MODE | " + string(race_.mode() == RaceMode::Ticks ? "one step per tick" : "same fraction done");
        f.put(0, 0, title, th.header);
        string tk = "TICK " + to_string(race_.ticks());
        f.put(W - textWidth(tk), 0, tk, th.header);
        f.put(0, 1, g.repeat(g.h, W), th.border);
        int half = (W - 1) / 2;
        int footer = H - 7;
        drawPane(f, 0, half - 1, footer, race_.paneA(), race_.nameA(), infoA_, paneA_, th, g, race_.fractionA());
        for (int y = 2; y < footer; y++) f.put(half, y, g.v, th.border);
        drawPane(f, half + 1, W - half - 1, footer, race_.paneB(), race_.nameB(), infoB_, paneB_, th, g, race_.fractionB());
        f.put(0, footer, g.repeat(g.h, W), th.border);
        if (race_.finished()) {
            drawComparisonPane(f, 0, half, footer + 1, race_.paneA(), infoA_, th);
            drawComparisonPane(f, half + 1, W - half - 1, footer + 1, race_.paneB(), infoB_, th);
            int fs = race_.fewerSteps();
            f.put(0, H - 1, fs == 0 ? "Both algorithms needed the same number of steps." :
                  "Fewer steps: " + (fs < 0 ? race_.nameA() : race_.nameB()) + ". Compare the per-algorithm metrics above.", th.accent);
        } else {
            vector<string> la = wrapText(noteOf(race_.paneA()), half - 2), lb = wrapText(noteOf(race_.paneB()), W - half - 2);
            for (int i = 0; i < 3 && i < (int)la.size(); i++) f.put(0, footer + 1 + i, la[i], th.normal);
            for (int i = 0; i < 3 && i < (int)lb.size(); i++) f.put(half + 1, footer + 1 + i, lb[i], th.normal);
            drawControlsLine(f, W, H, th);
        }
        return f;
    }

    // 0 = continue, 1 = back
    int handleKey(int key) {
        if (paneA_.handleKey(key)) { paneB_.panX = paneA_.panX; paneB_.panY = paneA_.panY; paneB_.viewMode = paneA_.viewMode; return 0; }
        switch (key) {
        case 'q': case 'Q': case K_ESC: return 1;
        case 'p': case 'P': if (race_.finished()) race_.reset(); play_.play(); break;
        case K_SPACE: if (race_.finished()) race_.reset(); play_.toggle(); break;
        case 'n': case 'N': case K_RIGHT: play_.pause(); race_.tick(); break;
        case 'b': case 'B': case K_LEFT: play_.pause(); race_.back(); break;
        case 'r': case 'R': play_.pause(); race_.reset(); break;
        case 's': case 'S': play_.cycleSpeed(); break;
        case 'm': case 'M': race_.setMode(race_.mode() == RaceMode::Ticks ? RaceMode::Progress : RaceMode::Ticks); play_.pause(); break;
        case 't': case 'T': settings_.theme = Theme::next(settings_.theme); settings_.save(); break;
        case 'u': case 'U': settings_.unicode = !settings_.unicode; settings_.save(); Terminal::clear(); break;
        case 'h': case 'H': { Theme th = Theme::make(settings_.theme); showHelp(th, glyphsFor(settings_)); Terminal::clear(); break; }
        case 'e': case 'E': exportBoth(); break;
        default: break;
        }
        return 0;
    }

    void run() {
        play_.level = settings_.speed;
        Terminal::clear();
        auto last = chrono::steady_clock::now();
        int lw = 0, lh = 0;
        bool dirty = true;
        for (;;) {
            int W = Terminal::width(), H = Terminal::height();
            if (W != lw || H != lh) { Terminal::clear(); lw = W; lh = H; dirty = true; }
            if (dirty) {
                Theme th = Theme::make(settings_.theme);
                Terminal::write("\x1b[H" + frameText(compose(W - 1, H - 1), th, H - 1));
                dirty = false;
            }
            int key = Terminal::readKey();
            if (key != K_NONE) { dirty = true; if (handleKey(key) == 1) return; }
            auto now = chrono::steady_clock::now();
            double dt = chrono::duration<double, milli>(now - last).count();
            last = now;
            int n = play_.advance(dt);
            for (int i = 0; i < n; i++) if (!race_.tick()) { play_.pause(); break; }
            if (n > 0) { if (race_.finished()) play_.pause(); dirty = true; }
            Terminal::sleepMs(15);
        }
    }

private:
    static string noteOf(const ExecutionEngine& e) {
        const Step* s = e.currentStep();
        return s ? s->note : (e.total() == 0 ? "No steps for this input." : "Ready.");
    }
    static void putClipped(Canvas& f, int x, int y, int w, const string& text, const Style& style) {
        if (w > 0) f.put(x, y, codeLineText(text, w), style);
    }

    void drawPane(Canvas& f, int x, int w, int footer, const ExecutionEngine& e, const string& name, const AlgorithmInfo& info,
                  VizPane& pane, const Theme& th, const GlyphSet& g, double fraction) {
        f.put(x, 2, name, th.accent);
        string pr = progressBar(fraction, max(4, min(20, w - 14))) + " " + to_string((int)(fraction * 100)) + "%";
        f.put(x + w - textWidth(pr), 2, pr, th.dim);
        putClipped(f, x, 3, w, "Step " + to_string(e.position()) + "/" + to_string(e.total()) +
                   "  Cmp " + to_string(e.stats().comparisons) + "  Swp " + to_string(e.stats().swaps), th.normal);
        putClipped(f, x, 4, w, "Assignments " + to_string(e.stats().assignments) + "  Accesses " + to_string(e.stats().accesses), th.normal);
        putClipped(f, x, 5, w, "Time B/A/W: " + info.best + " / " + info.average + " / " + info.worst, th.dim);
        putClipped(f, x, 6, w, "Space: " + info.space, th.dim);
        pane.draw(f, x, 7, w, footer - 7, e, th, g);
    }

    void drawComparisonPane(Canvas& f, int x, int w, int y, const ExecutionEngine& e, const AlgorithmInfo& info, const Theme& th) {
        Statistics stats = e.finalStats();
        putClipped(f, x, y, w, info.name, th.accent);
        putClipped(f, x, y + 1, w, "Time B/A/W: " + info.best + " / " + info.average + " / " + info.worst, th.normal);
        putClipped(f, x, y + 2, w, "Space: " + info.space + "  Steps: " + to_string(e.total()), th.normal);
        putClipped(f, x, y + 3, w, "Comparisons: " + to_string(stats.comparisons) + "  Swaps: " + to_string(stats.swaps), th.normal);
        putClipped(f, x, y + 4, w, "Assignments: " + to_string(stats.assignments) + "  Recorded: " + recordedTime(e.timeline().runtimeMs), th.normal);
    }

    void drawControlsLine(Canvas& f, int W, int H, const Theme& th) {
        string c = string("[P] ") + (play_.playing ? "Pause" : "Play") + "  [N] Next tick  [B] Back  [R] Reset  [S] Speed (" + Playback::name(play_.level) +
                   ")  [M] Mode  [Q] Back";
        f.put(0, H - 1, c.substr(0, W), th.dim);
    }
    void exportBoth() {
        ReportData rd;
        AlgorithmInfo a, b;
        a.name = race_.nameA(); a.category = "Race"; b.name = race_.nameB(); b.category = "Race";
        rd.info = &a; rd.timeline = &race_.paneA().timeline(); rd.comparison = race_.summary();
        ReportExporter::write("summary_report_A.md", rd);
        rd.info = &b; rd.timeline = &race_.paneB().timeline();
        ReportExporter::write("summary_report_B.md", rd);
    }

    Settings& settings_;
    RaceController& race_;
    Input input_;
    AlgorithmInfo infoA_, infoB_;
    Playback play_;
    VizPane paneA_, paneB_;
};

#endif
