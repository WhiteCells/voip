#include "logger.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& logFile, Level level) {
    configureSinks(logFile, level);
    boost::log::add_common_attributes();
    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
}

void Logger::setLogLevel(Level level) {
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= static_cast<boost::log::trivial::severity_level>(level)
    );
}

void Logger::log(Level level, const std::string& message) {
    BOOST_LOG_SEV(logger_, static_cast<boost::log::trivial::severity_level>(level)) << message;
}

void Logger::configureSinks(const std::string& logFile, Level level) {
    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;
    namespace keywords = boost::log::keywords;

    // ========== 文件日志 Sink ==========
    auto file_backend = boost::make_shared<sinks::text_file_backend>(
        keywords::file_name = logFile + "_%Y%m%d_%H%M%S.log",
        keywords::rotation_size = 5 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
    );

    auto file_sink = boost::make_shared<sinks::asynchronous_sink<sinks::text_file_backend>>(file_backend);
    file_sink->set_formatter(
        logging::expressions::stream
            << "[" << logging::expressions::attr<boost::posix_time::ptime>("TimeStamp")
            << "] [" << logging::trivial::severity << "] " << logging::expressions::smessage
    );

    logging::core::get()->add_sink(file_sink);

    // ========== 控制台日志 Sink ==========
    auto console_backend = boost::make_shared<sinks::text_ostream_backend>();
    console_backend->add_stream(boost::make_shared<std::ostream>(std::cout.rdbuf()));

    auto console_sink = boost::make_shared<sinks::asynchronous_sink<sinks::text_ostream_backend>>(console_backend);
    console_sink->set_formatter(
        logging::expressions::stream
            << "[" << logging::expressions::attr<boost::posix_time::ptime>("TimeStamp")
            << "] [" << logging::trivial::severity << "] " << logging::expressions::smessage
    );

    logging::core::get()->add_sink(console_sink);

    setLogLevel(level);
}
