#ifndef LOGGER_H
#define LOGGER_H

class Logger {
public:

    enum Level {
        ERROR,
        INFO,
        DEBUG,
        TRACE
    };

    static void setLevel(Level level) {
        Logger::level = level;
    }

    static Level getLevel() {
        return level;
    }

private:

    static Level level;
};

#endif /* LOGGER_H */
