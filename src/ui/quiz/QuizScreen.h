#ifndef QUIZSCREEN_H
#define QUIZSCREEN_H

// Quiz Mode screen ("Predict the Next Step"): shows the structure after the current step,
// asks what happens next (A-D), reveals the real step with its explanation, keeps the score.

#include <string>

#include "../../engine/quiz/QuizController.h"
#include "../Settings.h"
#include "../Terminal.h"
#include "../VisualizerScreen.h"
#include "../panels/ExplanationBar.h"

using namespace std;

class QuizScreen {
public:
    QuizScreen(Settings& s, QuizController& quiz, const string& algoName, ScoreTracker& sessionScore)
        : settings_(s), quiz_(quiz), name_(algoName), session_(sessionScore) {}

    // Builds the frame for the current phase. `q` is the open question (when asking) or the answered one.
    Canvas compose(int W, int H, const QuizQuestion* q, int chosen, bool revealed) {
        Theme th = Theme::make(settings_.theme);
        GlyphSet g = glyphsFor(settings_);
        Canvas f(W, H);
        if (W < 60 || H < 20) { f.put(0, 0, "Quiz Mode needs a terminal of at least 60 x 20.", th.error); return f; }
        f.put(0, 0, "ALGO VISUALIZER | QUIZ MODE | " + upperCase(name_), th.header);
        string sc = "Score " + to_string(session_.correct()) + "/" + to_string(session_.answered()) + "   Streak " + to_string(session_.streak());
        f.put(W - textWidth(sc), 0, sc, th.header);
        f.put(0, 1, g.repeat(g.h, W), th.border);
        int footer = H - 9;
        pane_.draw(f, 0, 2, W, footer - 2, quiz_.engine(), th, g);
        f.put(0, footer, g.repeat(g.h, W), th.border);
        const Step* cur = quiz_.engine().currentStep();
        if (cur && !cur->note.empty() && !revealed) f.put(0, footer + 1, "Last step: " + cur->note.substr(0, W - 12), th.dim);
        if (q) {
            f.put(0, footer + 2, "WHAT HAPPENS NEXT?   (question " + to_string(session_.answered() + 1) + ")", th.header);
            for (size_t i = 0; i < q->options.size(); i++) {
                Style st = th.normal;
                string mark = "  ";
                if (revealed) {
                    if ((int)i == q->correct) { st = th.role(Role::Sorted).style; mark = "+ "; }
                    else if ((int)i == chosen) { st = th.role(Role::Error).style; mark = "x "; }
                }
                f.put(0, footer + 3 + (int)i, mark + string(1, (char)('A' + i)) + ") " + q->options[i], st);
            }
            if (revealed) {
                bool ok = chosen == q->correct;
                f.put(0, footer + 7, string(ok ? "Correct! " : "Not quite. ") + q->explanation.substr(0, max(0, W - 12)), ok ? th.role(Role::Sorted).style : th.error);
                f.put(0, H - 1, "Press any key to continue,  Q to finish the quiz", th.dim);
            } else {
                f.put(0, H - 1, "Press A, B, C or D to answer   [H] Help  [T] Theme  [Q] Finish the quiz", th.dim);
            }
        }
        return f;
    }

    Canvas summary(int W, int H) {
        Theme th = Theme::make(settings_.theme);
        Canvas f(W, H);
        f.put(2, 1, "QUIZ RESULT - " + name_, th.header);
        f.put(2, 3, "This quiz:   " + to_string(thisQuizCorrect_) + " / " + to_string(thisQuizAsked_) + " correct", th.normal);
        f.put(2, 4, "Session:     " + session_.summary(), th.normal);
        f.put(2, 6, session_.grade(), th.accent);
        string hist;
        for (bool b : session_.history()) hist += b ? '#' : '.';
        f.put(2, 8, "Answers (# right, . wrong): " + hist.substr(hist.size() > 60 ? hist.size() - 60 : 0), th.dim);
        f.put(2, 10, "Press any key to return to the menu.", th.dim);
        return f;
    }

    void run() {
        while (!quiz_.finished()) {
            QuizQuestion q = quiz_.ask();
            int chosen = -1;
            // ---- ask
            for (;;) {
                int W = Terminal::width(), H = Terminal::height();
                Theme th = Theme::make(settings_.theme);
                Terminal::clear();
                Terminal::write(frameText(compose(W - 1, H - 1, &q, -1, false), th, H - 1));
                int k = Terminal::waitKey();
                if (k == 'q' || k == 'Q' || k == K_ESC) { finish(); return; }
                if (k == 't' || k == 'T') { settings_.theme = Theme::next(settings_.theme); settings_.save(); continue; }
                if (k == 'h' || k == 'H') { showHelp(th, glyphsFor(settings_)); continue; }
                if (pane_.handleKey(k)) continue;
                int idx = (k >= 'a' && k <= 'd') ? k - 'a' : (k >= 'A' && k <= 'D') ? k - 'A' : (k >= '1' && k <= '4') ? k - '1' : -1;
                if (idx >= 0 && idx < (int)q.options.size()) { chosen = idx; break; }
            }
            // ---- reveal
            bool ok = quiz_.answer(q, chosen);
            session_.record(ok);                 // the controller keeps this quiz's score, the session tracker outlives it
            thisQuizAsked_++; if (ok) thisQuizCorrect_++;
            int W = Terminal::width(), H = Terminal::height();
            Theme th = Theme::make(settings_.theme);
            Terminal::clear();
            Terminal::write(frameText(compose(W - 1, H - 1, &q, chosen, true), th, H - 1));
            int k = Terminal::waitKey();
            if (k == 'q' || k == 'Q' || k == K_ESC) { finish(); return; }
        }
        finish();
    }

private:
    void finish() {
        int W = Terminal::width(), H = Terminal::height();
        Theme th = Theme::make(settings_.theme);
        Terminal::clear();
        Terminal::write(frameText(summary(W - 1, H - 1), th, H - 1));
        Terminal::waitKey();
    }
    Settings& settings_;
    QuizController& quiz_;
    string name_;
    ScoreTracker& session_;
    VizPane pane_;
    int thisQuizAsked_ = 0, thisQuizCorrect_ = 0;
};

#endif
