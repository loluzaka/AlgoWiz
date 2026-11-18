#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

// Input validation. The parse*() functions are pure (string in, value or message out) so the
// tests can exercise every invalid case without a terminal. The ask*() functions wrap them in a
// prompt loop that shows an understandable message and asks again; typing "q" (or an empty
// line where allowed) cancels. Nothing here can crash on bad user input.

#include <climits>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Terminal.h"

using namespace std;

namespace InputManager {

inline string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}

// Parses one integer in [lo, hi]. Returns "" on success, otherwise the message to show.
inline string parseInt(const string& text, int lo, int hi, int& out) {
    string t = trim(text);
    if (t.empty()) return "Please enter a number (the line was empty).";
    size_t i = (t[0] == '-' || t[0] == '+') ? 1 : 0;
    if (i >= t.size()) return "\"" + t + "\" is not a number.";
    for (size_t k = i; k < t.size(); k++)
        if (t[k] < '0' || t[k] > '9') return "\"" + t + "\" is not a whole number.";
    if (t.size() - i > 9) return "That number is too large.";
    long long v = atoll(t.c_str());
    if (v < lo || v > hi) return "Please enter a number between " + to_string(lo) + " and " + to_string(hi) + ".";
    out = (int)v;
    return "";
}

// Space or comma separated integers, each in [lo, hi], count in [minCount, maxCount].
inline string parseIntList(const string& text, int minCount, int maxCount, int lo, int hi, vector<int>& out) {
    string t = text;
    for (char& c : t) if (c == ',' || c == ';') c = ' ';
    istringstream is(t);
    string tok;
    vector<int> v;
    while (is >> tok) {
        int x;
        string err = parseInt(tok, lo, hi, x);
        if (!err.empty()) return err;
        v.push_back(x);
    }
    if ((int)v.size() < minCount) return minCount == 1 ? "Please enter at least one number." : "Please enter at least " + to_string(minCount) + " numbers.";
    if ((int)v.size() > maxCount) return "Please enter at most " + to_string(maxCount) + " numbers (you entered " + to_string(v.size()) + ").";
    out = v;
    return "";
}

inline string parseWords(const string& text, int minCount, int maxCount, size_t maxLen, vector<string>& out) {
    string t = text;
    for (char& c : t) if (c == ',' || c == ';') c = ' ';
    istringstream is(t);
    string w;
    vector<string> v;
    while (is >> w) {
        if (w.size() > maxLen) return "The word \"" + w + "\" is longer than " + to_string(maxLen) + " characters.";
        v.push_back(w);
    }
    if ((int)v.size() < minCount) return "Please enter at least " + to_string(minCount) + " word(s).";
    if ((int)v.size() > maxCount) return "Please enter at most " + to_string(maxCount) + " words.";
    out = v;
    return "";
}

inline vector<int> randomValues(int count, int lo, int hi, unsigned seed = std::random_device{}()) {
    mt19937 rng(seed);
    vector<int> v;
    for (int i = 0; i < count; i++) v.push_back(lo + (int)(rng() % (unsigned)(hi - lo + 1)));
    return v;
}

// ---------------------------------------------------------------- interactive
// Each returns false if the user cancelled (typed "q").
inline bool askInt(const string& prompt, int lo, int hi, int& out, int defaultValue = INT_MIN) {
    for (;;) {
        string line = trim(Terminal::readLine(prompt + (defaultValue != INT_MIN ? " [" + to_string(defaultValue) + "]" : "") + " (q = cancel): "));
        if (line == "q" || line == "Q") return false;
        if (line.empty() && defaultValue != INT_MIN) { out = defaultValue; return true; }
        string err = parseInt(line, lo, hi, out);
        if (err.empty()) return true;
        Terminal::write("  ! " + err + "\n");
    }
}
inline bool askIntList(const string& prompt, int minCount, int maxCount, int lo, int hi, vector<int>& out) {
    for (;;) {
        string line = trim(Terminal::readLine(prompt + " (q = cancel): "));
        if (line == "q" || line == "Q") return false;
        string err = parseIntList(line, minCount, maxCount, lo, hi, out);
        if (err.empty()) return true;
        Terminal::write("  ! " + err + "\n");
    }
}
inline bool askWords(const string& prompt, int minCount, int maxCount, vector<string>& out) {
    for (;;) {
        string line = trim(Terminal::readLine(prompt + " (q = cancel): "));
        if (line == "q" || line == "Q") return false;
        string err = parseWords(line, minCount, maxCount, 14, out);
        if (err.empty()) return true;
        Terminal::write("  ! " + err + "\n");
    }
}
inline bool askText(const string& prompt, string& out) {
    out = trim(Terminal::readLine(prompt + " (q = cancel): "));
    return !(out == "q" || out == "Q");
}
// Presents numbered choices 1..n; returns the index (0-based) or -1 when cancelled.
inline int askChoice(const string& title, const vector<string>& options) {
    Terminal::write(title + "\n");
    for (size_t i = 0; i < options.size(); i++) Terminal::write("  " + to_string(i + 1) + ". " + options[i] + "\n");
    int n = 0;
    if (!askInt("Choose", 1, (int)options.size(), n)) return -1;
    return n - 1;
}

} // namespace InputManager

#endif
