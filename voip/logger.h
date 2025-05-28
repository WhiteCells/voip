#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <memory>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
// #include <mutex>

class Logger
{
public:
    static void init(
        std::string title = "logger",
        unsigned que_size = 8192,
        unsigned thread_cnt = 1,
        const std::string &log_dir = "logs",
        unsigned max_byte = 1048576 * 5,
        unsigned max_save = 3)
    {
        spdlog::init_thread_pool(que_size, thread_cnt);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] [thread %t] [%s:%# %!] %v");

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;

#if defined(_WIN32)
        localtime_s(&tm_now, &now_c);
#else
        localtime_r(&now_c, &tm_now);
#endif
        std::ostringstream oss;
        oss << log_dir << "/"
            << title << "_"
            << std::put_time(&tm_now, "%Y-%m-%d_%H-%M-%S") << ".log";
        std::string log_path = oss.str();

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, max_byte, max_save);
        // file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e %z] [%^%L%$] [thread %t] %v");
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] [thread %t] [%s:%# %!] %v");

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

        logger = std::make_shared<spdlog::async_logger>(
            title,
            sinks.begin(), sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);

        spdlog::register_logger(logger);
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
    }

    // template <typename... Args>
    // static void debug(Args &&...args)
    // {
    //     get()->debug(std::forward<Args>(args)...);
    // }

    template <typename... Args>
    static void debug(const spdlog::source_loc &loc, fmt::format_string<Args...> fmt, Args &&...args)
    {
        // get()->debug(std::forward<Args>(args)...);
        get()->log(loc, spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(const spdlog::source_loc &loc, fmt::format_string<Args...> fmt, Args &&...args)
    {
        // get()->info(std::forward<Args>(args)...);
        get()->log(loc, spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(const spdlog::source_loc &loc, fmt::format_string<Args...> fmt, Args &&...args)
    {
        // get()->warn(std::forward<Args>(args)...);
        get()->log(loc, spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(const spdlog::source_loc &loc, fmt::format_string<Args...> fmt, Args &&...args)
    {
        // get()->error(std::forward<Args>(args)...);
        get()->log(loc, spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void critical(const spdlog::source_loc &loc, fmt::format_string<Args...> fmt, Args &&...args)
    {
        // get()->critical(std::forward<Args>(args)...);
        get()->log(loc, spdlog::level::critical, fmt, std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> get()
    {
        // std::call_once(init_flag, []() {
        //     init();
        // });
        // if (!logger) {
        //     init();
        // }
        assert(logger);
        return logger;
    }

private:
    static std::shared_ptr<spdlog::logger> logger;
    // static std::once_flag init_flag;
};

// std::once_flag Logger::init_flag;

// #define LOG_DEBUG(fmt, ...) Logger::debug(fmt, ##__VA_ARGS__)
// #define LOG_INFO(fmt, ...) Logger::info(fmt, ##__VA_ARGS__)
// #define LOG_WARN(fmt, ...) Logger::warn(fmt, ##__VA_ARGS__)
// #define LOG_ERROR(fmt, ...) Logger::error(fmt, ##__VA_ARGS__)
// #define LOG_CRITICAL(fmt, ...) Logger::critical(fmt, ##__VA_ARGS__)

#define LOG_DEBUG(...)    Logger::debug(spdlog::source_loc {__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_INFO(...)     Logger::info(spdlog::source_loc {__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_WARN(...)     Logger::warn(spdlog::source_loc {__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_ERROR(...)    Logger::error(spdlog::source_loc {__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_CRITICAL(...) Logger::critical(spdlog::source_loc {__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

#endif // _LOGGER_H_
