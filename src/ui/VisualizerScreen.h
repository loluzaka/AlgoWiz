#ifndef VISUALIZERSCREEN_H
#define VISUALIZERSCREEN_H

// The main visualization screen (spec section 4): header, picture | information,
// the CURRENT OPERATION bar and the key help. compose() builds one frame as a Canvas (pure,
// also used by the tests and the --dump command); run() is the interactive loop around it.

#include <chrono>
#include <string>

#include "../core/AlgorithmCatalog.h"
#include "../engine/Playback.h"
#include "../engine/report/ReportExporter.h"
#include "InputManager.h"
#include "Settings.h"
#include "Terminal.h"
#include "VizPane.h"
#include "dialogs/HelpScreen.h"
#include "dialogs/HelpScreen.h"
#include "panels/CodePanel.h"
#include "panels/ExplanationBar.h"
#include "panels/InfoPanel.h"

using namespace std;

inline GlyphSet glyphsFor(const Settings& s) { return s.unicode ? GlyphSet::utf8() : GlyphSet::ascii(); }

// A finished frame as text for the terminal (ANSI) or for monochrome output.
inline string frameText(const Canvas& f, const Theme& th, int rows) { return th.color ? f.toAnsi(rows) : f.toPlain(rows, false); }

inline string upperCase(string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }

class VisualizerScreen {
public:
    enum class Result { Back, NewInput };

    VisualizerScreen(Settings& settings, const AlgorithmCatalog&, const CatalogEntry& entry, const AlgorithmInfo& info)
        : settings_(settings), entry_(entry), info_(info) {}

    void load(Timeline tl) {
        engine_.load(std::move(tl));
        playback_ = Playback();
        playback_.level = settings_.speed;
        pane_.panX = pane_.panY = 0;
        dirty_ = true;
    }
    ExecutionEngine& engine() { return engine_; }
    Playback& playback() { return playback_; }
    VizPane& pane() { return pane_; }
    void setStatus(const string& s) { status_ = s; statusUntil_ = chrono::steady_clock::now() + chrono::seconds(4); dirty_ = true; }

    // ------------------------------------------------------------- one frame
    Canvas compose(int W, int H) {
        Theme th = Theme::make(settings_.theme);
        GlyphSet g = glyphsFor(settings_);
        Canvas f(W, H);
        if (W < 60 || H < 18) {
            f.put(0, 0, "Terminal too small: need at least 60 x 18 (now " + to_string(W) + " x " + to_string(H) + ").", th.error);
            f.put(0, 1, "Enlarge the window or press Q to go back.", th.normal);
            return f;
        }
        string left = "ALGO VISUALIZER | " + upperCase(entry_.category) + " > " + upperCase(entry_.name);
        string right = "STEP " + to_string(engine_.position()) + " / " + to_string(engine_.total());
        f.put(0, 0, left, th.header);
        f.put(max(textWidth(left) + 2, W - textWidth(right)), 0, right, th.header);
        f.put(0, 1, g.repeat(g.h, W), th.border);

        int footerTop = H - 7;
        int mainTop = 2, mainH = footerTop - mainTop;
        int infoW = W >= 120 ? 38 : (W >= 90 ? 32 : 0);
        int vizW = W - (infoW ? infoW + 1 : 0);
        if (!engine_.timeline().error.empty()) {
            f.put(0, mainTop, "This input cannot be run:", th.error);
            auto lines = wrapText(engine_.timeline().error, vizW - 1);
            for (size_t i = 0; i < lines.size() && (int)i < mainH - 3; i++) f.put(0, mainTop + 2 + (int)i, lines[i], th.normal);
            f.put(0, mainTop + 3 + (int)lines.size(), "Press I to enter different data.", th.dim);
        } else {
            pane_.draw(f, 0, mainTop, vizW - 1, mainH, engine_, th, g);
        }
        if (infoW) {
            int ix = W - infoW;
            for (int y = mainTop; y < footerTop; y++) f.put(ix - 1, y, g.v, th.border);
            if (settings_.showCode) {
                int infoH = min(mainH, 13);
                drawInfoPanel(f, ix, mainTop, infoW, infoH, info_, engine_, th, true);
                int codeH = mainH - infoH - 1;
                if (codeH > 0) drawCodePanel(f, ix, mainTop + infoH + 1, infoW, codeH, info_, engine_, th);
            } else {
                drawInfoPanel(f, ix, mainTop, infoW, mainH, info_, engine_, th, false);
            }
        }
        f.put(0, footerTop, g.repeat(g.h, W), th.border);
        drawExplanationBar(f, 0, footerTop + 1, W, 3, engine_.currentStep(), engine_.position(), engine_.total(), th);
        drawControls(f, 0, footerTop + 4, W, 2, playback_.playing, th);
        string st = "Speed: " + Playback::name(playback_.level) + (playback_.fastForward ? " x4" : "") + "   Theme: " + Theme::name(settings_.theme) +
                    (pane_.viewModeCount() > 1 ? "   View: " + pane_.viewModeName() : "");
        if (!status_.empty()) st = status_;
        f.put(max(0, W - textWidth(st)), footerTop + 6, st, status_.empty() ? th.dim : th.accent);
        return f;
    }

    // ----------------------------------------------------------- interactive
    Result run() {
        Terminal::clear();
        auto last = chrono::steady_clock::now();
        int lastW = 0, lastH = 0;
        for (;;) {
            int W = Terminal::width(), H = Terminal::height();
            if (W != lastW || H != lastH) { Terminal::clear(); lastW = W; lastH = H; dirty_ = true; }
            if (!status_.empty() && chrono::steady_clock::now() > statusUntil_) { status_.clear(); dirty_ = true; }
            if (dirty_) {
                Theme th = Theme::make(settings_.theme);
                Canvas f = compose(W - 1, H - 1);
                Terminal::write("\x1b[H" + frameText(f, th, H - 1));
                dirty_ = false;
            }
            int key = Terminal::readKey();
            if (key != K_NONE) {
                dirty_ = true;
                int r = handleKey(key, H);
                if (r == 1) return Result::Back;
                if (r == 2) return Result::NewInput;
            }
            auto now = chrono::steady_clock::now();
            double dt = chrono::duration<double, milli>(now - last).count();
            last = now;
            int n = playback_.advance(dt);
            if (n > 0) {
                for (int i = 0; i < n && !engine_.atEnd(); i++) engine_.next();
                if (engine_.atEnd()) { playback_.pause(); playback_.skipAnimation = false; }
                dirty_ = true;
            }
            Terminal::sleepMs(15);
        }
    }

    // Returns 0 = continue, 1 = back, 2 = new input. Public so the tests can drive the screen without a terminal.
    int handleKey(int key, int termHeight = 24) {
        dirty_ = true;
        if (pane_.handleKey(key)) return 0;
        switch (key) {
        case 'q': case 'Q': case K_ESC: return 1;
        case 'i': case 'I': return 2;
        case 'p': case 'P': if (engine_.atEnd()) engine_.reset(); playback_.play(); break;
        case K_SPACE: if (engine_.atEnd()) engine_.reset(); playback_.toggle(); break;
        case 'n': case 'N': case K_RIGHT: playback_.pause(); engine_.next(); break;
        case 'b': case 'B': case K_LEFT: playback_.pause(); engine_.previous(); break;
        case 'r': case 'R': case K_HOME: playback_.pause(); engine_.reset(); break;
        case K_END: playback_.pause(); engine_.toEnd(); break;
        case 's': case 'S': playback_.cycleSpeed(); settings_.speed = playback_.level; settings_.save(); break;
        case 'f': case 'F': playback_.fastForward = !playback_.fastForward; break;
        case 'k': case 'K': playback_.skipAnimation = true; playback_.play(); break;
        case 'c': case 'C': settings_.showCode = !settings_.showCode; settings_.save(); break;      // never touches playback
        case 't': case 'T': settings_.theme = Theme::next(settings_.theme); settings_.save(); break;
        case 'u': case 'U': settings_.unicode = !settings_.unicode; settings_.save(); Terminal::clear(); break;
        case 'h': case 'H': { Theme th = Theme::make(settings_.theme); showHelp(th, glyphsFor(settings_)); Terminal::clear(); break; }
        case 'e': case 'E': exportReport(); break;
        case 'j': case 'J': {
            Terminal::write("\x1b[" + to_string(termHeight) + ";1H\x1b[2K");
            string line = InputManager::trim(Terminal::readLine("Jump to step (0-" + to_string(engine_.total()) + "): "));
            int n;
            if (InputManager::parseInt(line, 0, (int)engine_.total(), n).empty()) { playback_.pause(); engine_.jumpTo((size_t)n); }
            else setStatus("Not a valid step number.");
            Terminal::clear();
            break;
        }
        default: break;
        }
        return 0;
    }

    string snapshot() {
        Theme th = Theme::make(ThemeId::Mono);
        GlyphSet g = GlyphSet::ascii();
        VizPane p;
        Canvas c(90, 60);
        p.draw(c, 0, 0, 90, 60, engine_, th, g);
        return c.toPlain(c.usedHeight());
    }
    void exportReport() {
        ReportData rd;
        rd.info = &info_; rd.timeline = &engine_.timeline(); rd.snapshot = snapshot();
        string err = ReportExporter::write("summary_report.md", rd);
        setStatus(err.empty() ? "Saved summary_report.md" : err);
    }

private:
    Settings& settings_;
    CatalogEntry entry_;
    AlgorithmInfo info_;
    ExecutionEngine engine_;
    Playback playback_;
    VizPane pane_;
    bool dirty_ = true;
    string status_;
    chrono::steady_clock::time_point statusUntil_;
};

#endif
