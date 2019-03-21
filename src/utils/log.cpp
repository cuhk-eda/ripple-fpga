#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "log.h"

using namespace std;

//////////TIMER FUNTIONS//////////

namespace timer {
timer::timer() { start(); }
void timer::start() { _start = clock::now(); }
double timer::elapsed() const { return std::chrono::duration<double>(clock::now() - _start).count(); }
}  // namespace timer

timer::timer tstamp;

inline ostream& operator<<(ostream& os, const timer::timer& t) {
    return os << "[" << setprecision(3) << setw(8) << fixed << t.elapsed() << " ] ";
}

inline string to_string(const timer::timer& t) {
    ostringstream oss;
    oss << t;
    return oss.str();
}

///////////////LOGGING FUNCTIONS///////////////////////

ostream& log(ostream& os) {
    os << to_string(tstamp);
    return os;
}

int _active_levels = 0;
int _color_levels = LOG_FATAL | LOG_ERROR | LOG_WARN;

void init_log(int log_level) { _active_levels = log_level; }

void printlog(int level, const char* format, ...) {
    if ((level & _active_levels) == 0) return;
    char c;
    switch (level) {
        case LOG_FATAL:
            c = '!';
            printf(ESC_BG_RED);
            break;
        case LOG_ERROR:
            c = 'E';
            printf(ESC_RED);
            break;
        case LOG_WARN:
            c = 'W';
            printf(ESC_YELLOW);
            break;
        case LOG_NOTICE:
            c = 'N';
            break;
        default:
            c = ' ';
            break;
    }
    printf(LOG_PREFIX, tstamp.elapsed(), c);
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    if (level & _color_levels) printf(ESC_RESET);
    printf(LOG_SUFFIX);
    fflush(stdout);
}