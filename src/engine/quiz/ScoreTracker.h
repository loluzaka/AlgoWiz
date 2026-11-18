#ifndef SCORETRACKER_H
#define SCORETRACKER_H

// Keeps the score of Quiz Mode: answered, correct, current and best streak, percentage.
// The tracker lives as long as the program (not per quiz), so the score persists across
// several quizzes in one session; saveTo() additionally appends a line to a text file.

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class ScoreTracker {
public:
    void record(bool correct) {
        answered_++;
        history_.push_back(correct);
        if (correct) { correct_++; streak_++; if (streak_ > best_) best_ = streak_; }
        else streak_ = 0;
    }
    void reset() { answered_ = correct_ = streak_ = best_ = 0; history_.clear(); }

    int answered() const { return answered_; }
    int correct() const { return correct_; }
    int streak() const { return streak_; }
    int bestStreak() const { return best_; }
    double percent() const { return answered_ ? 100.0 * correct_ / answered_ : 0.0; }
    const vector<bool>& history() const { return history_; }

    string summary() const {
        ostringstream o;
        o << correct_ << " / " << answered_ << " correct (" << (int)(percent() + 0.5) << "%), best streak " << best_;
        return o.str();
    }
    string grade() const {
        if (answered_ == 0) return "No answers yet";
        double p = percent();
        return p >= 90 ? "Excellent - you can predict this algorithm!" : p >= 70 ? "Good - you understand the main idea." : p >= 50 ? "Fair - replay the algorithm and watch the explanations." : "Keep practising - step through the algorithm slowly first.";
    }
    bool saveTo(const string& path, const string& label) const {
        ofstream f(path, ios::app);
        if (!f) return false;
        f << label << ": " << summary() << "\n";
        return true;
    }

private:
    int answered_ = 0, correct_ = 0, streak_ = 0, best_ = 0;
    vector<bool> history_;
};

#endif
