#pragma once

// #define ENABLE_THREAD_LOGGING 1

#ifdef CLIENT_ONLY
#undef ENABLE_THREAD_LOGGING
#endif

// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#ifndef CLIENT_ONLY
#include "cef_includes.h"
#endif

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"

#define TRACE(...)     SPDLOG_LOGGER_TRACE(logger->current(), __VA_ARGS__)
#define DEBUG(...)     SPDLOG_LOGGER_DEBUG(logger->current(), __VA_ARGS__)
#define INFO(...)      SPDLOG_LOGGER_INFO(logger->current(), __VA_ARGS__)
#define ERROR(...)     SPDLOG_LOGGER_ERROR(logger->current(), __VA_ARGS__)
#define CRITICAL(...)  SPDLOG_LOGGER_CRITICAL(logger->current(), __VA_ARGS__)

#if defined ENABLE_THREAD_LOGGING && not defined(CLIENT_ONLY)
#define LOG_CURRENT_THREAD() \
        if (CefCurrentlyOn(TID_UI))  { \
            TRACE("Current Thread: UI"); \
        } else if (CefCurrentlyOn(TID_IO))  { \
            TRACE("Current Thread: IO"); \
        } else if (CefCurrentlyOn(TID_FILE_BACKGROUND))  { \
            TRACE("Current Thread: FILE_BACKGROUND"); \
        } else if (CefCurrentlyOn(TID_FILE_USER_VISIBLE)) { \
            TRACE("Current Thread: TID_FILE_USER_VISIBLE"); \
        } else if (CefCurrentlyOn(TID_FILE_USER_BLOCKING))  { \
            TRACE("Current Thread: FILE_USER_BLOCKING"); \
        } else if (CefCurrentlyOn(TID_PROCESS_LAUNCHER))  { \
            TRACE("Current Thread: PROCESS_LAUNCHER"); \
        } else if (CefCurrentlyOn(TID_RENDERER)) { \
            TRACE("Current Thread: RENDERER"); \
        }
#else
#define LOG_CURRENT_THREAD() (void)0
#endif

class Logger {

public:
    Logger();

    ~Logger();

    // Must be called before setting the desired level
    void switchToFileLogger(std::string filename);

    void set_level(spdlog::level::level_enum level) {
        _logger->set_level(level);
    }

    inline bool isTraceEnabled() {
        return _logger->level() == spdlog::level::trace;
    }

    inline bool isDebugEnabled() {
        return _logger->level() == spdlog::level::debug;
    }

    inline spdlog::level::level_enum level() {
        return _logger->level();
    }

    inline std::shared_ptr<spdlog::logger> current() {
        return _logger;
    }

    inline bool switchedToFile() {
        return this->_switchedToFile;
    }

    inline void shutdown() {
        _logger->flush();
        spdlog::drop("cefbrowser");
        spdlog::drop("browser");
    }

private:
    std::shared_ptr<spdlog::logger> _logger;
    bool _switchedToFile = false;
};

extern Logger* logger;
