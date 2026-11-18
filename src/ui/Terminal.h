#ifndef TERMINAL_H
#define TERMINAL_H

// The ONLY file pair that talks to the operating system's console: size, key input,
// ANSI/UTF-8 set-up, sleeping. Everything above it works with plain strings and key codes,
// which keeps the rest of the program portable and testable without a terminal.

#include <string>

using namespace std;

enum Key {
    K_NONE = 0, K_BACKSPACE = 8, K_TAB = 9, K_ENTER = 13, K_ESC = 27, K_SPACE = 32,
    K_UP = 1001, K_DOWN, K_LEFT, K_RIGHT, K_PGUP, K_PGDN, K_HOME, K_END
};

namespace Terminal {
void init();                         // enable ANSI escape codes and UTF-8 output, hide the cursor
void shutdown();                     // restore the console
bool isInteractive();                // stdin and stdout are a real terminal
int  width();                        // columns (80 if unknown)
int  height();                       // rows (24 if unknown)
bool supportsUnicode();              // best-effort detection of a UTF-8 capable terminal
int  readKey();                      // non-blocking: returns K_NONE when no key is waiting
int  waitKey();                      // blocking
string readLine(const string& prompt);   // cooked-mode line input with the cursor visible
void write(const string& s);         // write + flush
void clear();                        // clear the screen
void home();                         // move the cursor to the top-left corner
void sleepMs(int ms);
}

#endif
