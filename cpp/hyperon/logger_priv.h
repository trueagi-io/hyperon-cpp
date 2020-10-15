#ifndef LOGGER_PRIV_H
#define LOGGER_PRIV_H

#include <iostream>
#include <iomanip>

#include "logger.h"

class LoggerImpl {
public:

    template<typename T>
    LoggerImpl& operator<<(const T& data) {
        if (level <= Logger::getLevel()) {
            std::clog << data;
        }
        return *this;
    }

    LoggerImpl& operator<<(std::ostream& (&endl)(std::ostream& os)) {
        if (level <= Logger::getLevel()) {
            std::clog << endl;
        }
        return *this;
    }

    LoggerImpl(Logger::Level level) : level(level) { }

private:

    Logger::Level level;
};

namespace clog {

extern LoggerImpl error;
extern LoggerImpl info;
extern LoggerImpl debug;
extern LoggerImpl trace;

};

#define LOG_ERROR (clog::error << "ERROR: " << __func__ << ": ")
#define LOG_INFO (clog::info << "INFO:  " << __func__ << ": ")
#define LOG_DEBUG (clog::debug << "DEBUG: " << __func__ << ": ")
#define LOG_TRACE (clog::error << "TRACE: " << __func__ << ": ")

#endif /* LOGGER_PRIV_H */
