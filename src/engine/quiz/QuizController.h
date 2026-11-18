#ifndef QUIZCONTROLLER_H
#define QUIZCONTROLLER_H

// "Predict the Next Step" quiz logic. The controller shows the state after the current step,
// asks what the NEXT step will be (multiple choice built from the real timeline), checks the
// answer, updates the ScoreTracker and moves the engine forward. UI-free.

#include <algorithm>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../Describe.h"
#include "../ExecutionEngine.h"
#include "ScoreTracker.h"

using namespace std;

struct QuizQuestion {
    size_t position = 0;             // engine position when the question was asked
    vector<string> options;          // 2-4 choices
    int correct = 0;                 // index into options
    string explanation;              // note of the real next step, shown after answering
};

class QuizController {
public:
    explicit QuizController(Timeline tl, unsigned seed = 12345) : rng_(seed) {
        engine_.load(std::move(tl));
        skipNarration();
    }

    ExecutionEngine& engine() { return engine_; }
    const ExecutionEngine& engine() const { return engine_; }
    ScoreTracker& score() { return score_; }
    const ScoreTracker& score() const { return score_; }
    bool finished() const { return engine_.atEnd(); }
    size_t questionsTotal() const {            // steps that can be asked about (narration-only steps are skipped)
        size_t n = 0;
        for (const Step& s : engine_.timeline().steps) if (s.op != OpType::Note) n++;
        return n;
    }

    // Builds the question about the next step. Precondition: !finished().
    QuizQuestion ask() {
        QuizQuestion q;
        const Step* next = engine_.nextStep();
        q.position = engine_.position();
        q.explanation = next ? next->note : "";
        string right = next ? describeStep(*next) : "";
        set<string> pool;
        for (const Step& s : engine_.timeline().steps) {
            string d = describeStep(s);
            if (s.op != OpType::Note && d != right) pool.insert(d);
        }
        vector<string> wrong(pool.begin(), pool.end());
        shuffle(wrong.begin(), wrong.end(), rng_);
        // synthetic distractors when the timeline is too uniform (e.g. only one kind of step)
        for (OpType op : {OpType::Swap, OpType::Compare, OpType::Assign, OpType::Highlight}) {
            if (wrong.size() >= 3) break;
            Step fake; fake.op = op;
            if (next) for (const RoleTag& r : next->roles) fake.roles.push_back(r);
            string d = describeStep(fake);
            if (d != right && find(wrong.begin(), wrong.end(), d) == wrong.end()) wrong.push_back(d);
        }
        q.options.push_back(right);
        for (size_t i = 0; i < wrong.size() && q.options.size() < 4; i++) q.options.push_back(wrong[i]);
        shuffle(q.options.begin(), q.options.end(), rng_);
        q.correct = (int)(find(q.options.begin(), q.options.end(), right) - q.options.begin());
        return q;
    }

    // Records the answer, advances to the revealed step and returns whether it was right.
    bool answer(const QuizQuestion& q, int choice) {
        bool ok = choice == q.correct;
        score_.record(ok);
        engine_.next();
        skipNarration();
        return ok;
    }
    // Restarts the same timeline; the score is kept (session persistence).
    void restart() { engine_.reset(); skipNarration(); }

private:
    void skipNarration() {            // pure Note steps carry no state change: nothing to predict
        while (!engine_.atEnd() && engine_.nextStep()->op == OpType::Note) engine_.next();
    }
    ExecutionEngine engine_;
    ScoreTracker score_;
    mt19937 rng_;
};

#endif
