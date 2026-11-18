#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <string>

using namespace std;

enum class SpeedLevel { Slow = 0, Normal = 1, Fast = 2, Turbo = 3 };

// Qt-free playback clock. The UI calls advance(elapsedMs) from its main loop and
// receives how many steps to move forward. It owns no engine, so Race Mode can
// drive several engines from one clock.
class Playback {
public:
    bool       playing = false;
    SpeedLevel level = SpeedLevel::Normal;
    bool       fastForward = false;   // x4 while held / toggled
    bool       skipAnimation = false; // play straight to the end in one tick

    static int intervalMs(SpeedLevel l) {
        switch (l) {
        case SpeedLevel::Slow:   return 1200;
        case SpeedLevel::Normal: return 600;
        case SpeedLevel::Fast:   return 250;
        default:                 return 80;
        }
    }
    static string name(SpeedLevel l) {
        switch (l) {
        case SpeedLevel::Slow:   return "Slow";
        case SpeedLevel::Normal: return "Normal";
        case SpeedLevel::Fast:   return "Fast";
        default:                 return "Turbo";
        }
    }
    void cycleSpeed() { level = static_cast<SpeedLevel>((static_cast<int>(level) + 1) % 4); }
    void toggle() { playing = !playing; accumulated = 0; }
    void play()   { playing = true; accumulated = 0; }
    void pause()  { playing = false; }

    // Returns the number of steps to advance. skipAnimation returns a huge count.
    int advance(double elapsedMs) {
        if (!playing) return 0;
        if (skipAnimation) return 1 << 30;
        double interval = intervalMs(level) / (fastForward ? 4.0 : 1.0);
        accumulated += elapsedMs;
        int n = 0;
        while (accumulated >= interval) { accumulated -= interval; n++; }
        return n;
    }
private:
    double accumulated = 0;
};

#endif
