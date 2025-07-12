#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

#define LOG_INFO(...) Logger::getFileLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getFileLogger()->warn(__VA_ARGS__)
#define LOG_ERR(...) Logger::getFileLogger()->error(__VA_ARGS__)

class Logger {
   public:
    static void init();

    static std::shared_ptr<spdlog::logger>& getConsoleLogger();
    static std::shared_ptr<spdlog::logger>& getFileLogger();

   private:
    inline static std::shared_ptr<spdlog::logger> consoleLogger = nullptr;
    inline static std::shared_ptr<spdlog::logger> fileLogger = nullptr;
};

inline void Logger::init() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    if (!consoleLogger) {
        consoleLogger = spdlog::stdout_color_mt("console");
        consoleLogger->set_level(spdlog::level::debug);
    }

    if (!fileLogger) {
        fileLogger = spdlog::basic_logger_mt("file", "server.log");
        fileLogger->set_level(spdlog::level::info);
    }
}

inline std::shared_ptr<spdlog::logger>& Logger::getConsoleLogger() {
    return consoleLogger;
}

inline std::shared_ptr<spdlog::logger>& Logger::getFileLogger() {
    return fileLogger;
}
