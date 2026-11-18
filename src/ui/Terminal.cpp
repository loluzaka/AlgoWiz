// windows.h must come first: it defines names (such as `byte`) that clash with
// `using namespace std;` if they are seen afterwards.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "Terminal.h"

namespace Terminal {

#ifndef _WIN32
static termios g_saved;
static bool g_raw = false;
static void setRaw(bool on) {
    if (on && !g_raw) {
        tcgetattr(STDIN_FILENO, &g_saved);
        termios t = g_saved;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        g_raw = true;
    } else if (!on && g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
        g_raw = false;
    }
}
#endif

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    setRaw(true);
#endif
    write("\x1b[?25l");
}

void shutdown() {
    write("\x1b[0m\x1b[?25h");
#ifndef _WIN32
    setRaw(false);
#endif
}

bool isInteractive() {
#ifdef _WIN32
    DWORD m;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &m) && GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &m);
#else
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

int width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return info.srWindow.Right - info.srWindow.Left + 1;
#else
    winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    return 80;
}

int height() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return info.srWindow.Bottom - info.srWindow.Top + 1;
#else
    winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) return ws.ws_row;
#endif
    return 24;
}

bool supportsUnicode() {
#ifdef _WIN32
    // Windows Terminal and VS Code set these; the classic conhost may render box characters badly.
    return std::getenv("WT_SESSION") != nullptr || std::getenv("TERM_PROGRAM") != nullptr || GetConsoleOutputCP() == CP_UTF8;
#else
    const char* l = std::getenv("LC_ALL");
    if (!l || !*l) l = std::getenv("LANG");
    return l && (std::string(l).find("UTF-8") != std::string::npos || std::string(l).find("utf8") != std::string::npos);
#endif
}

#ifdef _WIN32
static int decodeExtended() {
    switch (_getch()) {
    case 72: return K_UP;   case 80: return K_DOWN;  case 75: return K_LEFT;  case 77: return K_RIGHT;
    case 73: return K_PGUP; case 81: return K_PGDN;  case 71: return K_HOME;  case 79: return K_END;
    default: return K_NONE;
    }
}
int readKey() {
    if (!_kbhit()) return K_NONE;
    int c = _getch();
    if (c == 0 || c == 0xE0) return decodeExtended();
    return c;
}
int waitKey() {
    int c = _getch();
    if (c == 0 || c == 0xE0) return decodeExtended();
    return c;
}
#else
int readKey() {
    fd_set fds;
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    timeval tv = {0, 0};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) return K_NONE;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return K_NONE;
    if (c == 127) return K_BACKSPACE;
    if (c == 10) return K_ENTER;
    if (c != 27) return c;
    unsigned char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return K_ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return K_ESC;
    if (seq[0] == '[') switch (seq[1]) {
        case 'A': return K_UP;  case 'B': return K_DOWN; case 'C': return K_RIGHT; case 'D': return K_LEFT;
        case 'H': return K_HOME; case 'F': return K_END;
        case '5': read(STDIN_FILENO, &seq[2], 1); return K_PGUP;
        case '6': read(STDIN_FILENO, &seq[2], 1); return K_PGDN;
    }
    return K_NONE;
}
int waitKey() {
    for (;;) { int k = readKey(); if (k != K_NONE) return k; sleepMs(15); }
}
#endif

string readLine(const string& prompt) {
#ifndef _WIN32
    setRaw(false);
#endif
    write("\x1b[?25h" + prompt);
    std::string line;
    if (!std::getline(std::cin, line)) line = "q";
    write("\x1b[?25l");
#ifndef _WIN32
    setRaw(true);
#endif
    return line;
}

void write(const string& s) {
    std::fwrite(s.data(), 1, s.size(), stdout);
    std::fflush(stdout);
}

void clear() { write("\x1b[2J\x1b[H"); }
void home() { write("\x1b[H"); }
void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

} // namespace Terminal
