#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <boost/log/trivial.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

class Logger
{
public:
    enum class Level {
        Trace = boost::log::trivial::trace,
        Debug = boost::log::trivial::debug,
        Info = boost::log::trivial::info,
        Warn = boost::log::trivial::warning,
        Error = boost::log::trivial::error,
        Fatal = boost::log::trivial::fatal
    };

    static Logger &getInstance();

    void init(const std::string &logFile, Level level);
    void setLogLevel(Level level);
    void log(Level level, const std::string &message);

private:
    Logger() = default;
    void configureSinks(const std::string &logFile, Level level);

    boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_;
};

#define LOG_TRACE(msg) Logger::getInstance().log(Logger::Level::Trace, msg)
#define LOG_DEBUG(msg) Logger::getInstance().log(Logger::Level::Debug, msg)
#define LOG_INFO(msg)  Logger::getInstance().log(Logger::Level::Info, msg)
#define LOG_WARN(msg)  Logger::getInstance().log(Logger::Level::Warn, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(Logger::Level::Error, msg)
#define LOG_FATAL(msg) Logger::getInstance().log(Logger::Level::Fatal, msg)

#endif // LOGGER_H
