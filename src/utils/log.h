#pragma once

#include <cstdarg>
#include <chrono>

//////////TIMER FUNTIONS//////////

namespace timer {
class timer {
    typedef std::chrono::steady_clock clock;

private:
    clock::time_point _start;

public:
    timer();
    void start();
    double elapsed() const;  // seconds
};
}  // namespace timer

inline std::ostream& operator<<(std::ostream& os, const timer::timer& t);
inline std::string to_string(const timer::timer& t);

//////////ESCAPE CODE DEFINIATION//////////

#define ESC_RESET "\033[0m"
#define ESC_BOLD "\033[1m"
#define ESC_UNDERLINE "\033[4m"
#define ESC_BLINK "\033[5m"
#define ESC_REVERSE "\033[7m"
#define ESC_BLACK "\033[30m"
#define ESC_RED "\033[31m"
#define ESC_GREEN "\033[32m"
#define ESC_YELLOW "\033[33m"
#define ESC_BLUE "\033[34m"
#define ESC_MAGENTA "\033[35m"
#define ESC_CYAN "\033[36m"
#define ESC_WHITE "\033[37m"
#define ESC_DEFAULT "\033[39m"
#define ESC_BG_BLACK "\033[40m"
#define ESC_BG_RED "\033[41m"
#define ESC_BG_GREEN "\033[42m"
#define ESC_BG_YELLOW "\033[43m"
#define ESC_BG_BLUE "\033[44m"
#define ESC_BG_MAGENTA "\033[45m"
#define ESC_BG_CYAN "\033[46m"
#define ESC_BG_WHITE "\033[47m"
#define ESC_BG_DEFAULT "\033[49m"
#define ESC_OVERLINE "\033[53m"

///////////////LOGGING FUNCTIONS///////////////////////

std::ostream& log(std::ostream& os = std::cout);

#define LOG_DEBUG 1
#define LOG_VERBOSE 2
#define LOG_INFO 4
#define LOG_NOTICE 8
#define LOG_WARN 16
#define LOG_ERROR 32
#define LOG_FATAL 64
#define LOG_ALL 127
#define LOG_FAIL LOG_ERROR | LOG_FATAL
#define LOG_NORMAL LOG_ALL ^ (LOG_DEBUG | LOG_VERBOSE)

#define LOG_PREFIX "[%8.3lf ]%c "
#define LOG_SUFFIX "\n"

void init_log(int log_level = LOG_NORMAL);
void printlog(int level, const char* format, ...);