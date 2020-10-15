#include "logger_priv.h"

Logger::Level Logger::level = Logger::ERROR;

LoggerImpl clog::error(Logger::ERROR);
LoggerImpl clog::info(Logger::INFO);
LoggerImpl clog::debug(Logger::DEBUG);
LoggerImpl clog::trace(Logger::TRACE);

