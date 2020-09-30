#include "logger.h"

Logger::Level Logger::maxLevel = ERROR;

Logger clog::error(Logger::ERROR);
Logger clog::info(Logger::INFO);
Logger clog::debug(Logger::DEBUG);
Logger clog::trace(Logger::TRACE);

