#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <iomanip>

class Logger {
public:

    enum Level {
        ERROR,
        INFO,
        DEBUG,
        TRACE
    };

    template<typename T>
    Logger& operator<<(const T& data) {
        if (level <= maxLevel) {
            std::clog << data;
        }
        return *this;
    }

    Logger& operator<<(std::ostream& (&endl)(std::ostream& os)) {
        if (level <= maxLevel) {
            std::clog << endl;
        }
        return *this;
    }

    static void setLevel(Level level) {
        maxLevel = level;
    }

    Logger(Level level) : level(level) { }

private:
    Level level;

    static Level maxLevel;
};

namespace clog {
    extern Logger error;
    extern Logger info;
    extern Logger debug;
    extern Logger trace;
};

#endif /* LOGGER_H */
