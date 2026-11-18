#ifndef SETTINGS_H
#define SETTINGS_H

// Remembers theme, speed and a few display options between runs in "algoviz.ini"
// (key=value lines). This replaces QSettings from the Qt plan using only std::fstream.

#include <fstream>
#include <map>
#include <string>

#include "../engine/Playback.h"
#include "../render/theme/Theme.h"

using namespace std;

struct Settings {
    ThemeId theme = ThemeId::Dark;
    SpeedLevel speed = SpeedLevel::Normal;
    bool unicode = false;           // draw with Unicode box characters
    bool showCode = false;          // code panel visible

    static string defaultPath() { return "algoviz.ini"; }
    // Tests and --dump switch saving to the DEFAULT file off so they never litter the working directory.
    static bool& noDefaultSave() { static bool b = false; return b; }

    bool load(const string& path = defaultPath()) {
        ifstream f(path);
        if (!f) return false;
        string line;
        while (getline(f, line)) {
            size_t eq = line.find('=');
            if (eq == string::npos) continue;
            string k = line.substr(0, eq), v = line.substr(eq + 1);
            int n = v.empty() ? 0 : atoi(v.c_str());
            if (k == "theme" && n >= 0 && n <= 4) theme = static_cast<ThemeId>(n);
            else if (k == "speed" && n >= 0 && n <= 3) speed = static_cast<SpeedLevel>(n);
            else if (k == "unicode") unicode = n != 0;
            else if (k == "showCode") showCode = n != 0;
        }
        return true;
    }
    bool save(const string& path = defaultPath()) const {
        if (path == defaultPath() && noDefaultSave()) return false;
        ofstream f(path);
        if (!f) return false;
        f << "theme=" << static_cast<int>(theme) << "\nspeed=" << static_cast<int>(speed) << "\nunicode=" << (unicode ? 1 : 0)
          << "\nshowCode=" << (showCode ? 1 : 0) << "\n";
        return true;
    }
};

#endif
