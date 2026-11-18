// Test runner: executes every registered TEST_CASE (optionally filtered by a substring).
//   algoviz_tests            run everything
//   algoviz_tests sorting    run test cases whose name contains "sorting"
#include <iostream>
#include <string>

#include "TestFramework.h"
#include "../src/ui/Settings.h"

int main(int argc, char** argv) {
    Settings::noDefaultSave() = true;
    std::string filter = argc > 1 ? argv[1] : "";
    int ran = 0, failedCases = 0;
    for (const auto& t : tf::registry()) {
        if (!filter.empty() && t.name.find(filter) == std::string::npos) continue;
        int before = tf::failures();
        t.fn();
        ran++;
        if (tf::failures() != before) { failedCases++; std::cout << "[FAIL] " << t.name << "\n"; }
    }
    std::cout << ran << " test cases, " << tf::checks() << " checks, " << tf::failures() << " failed checks";
    if (failedCases) std::cout << " (" << failedCases << " failing cases)";
    std::cout << "\n";
    return tf::failures() == 0 ? 0 : 1;
}
