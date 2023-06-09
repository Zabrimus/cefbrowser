#include "logger.h"
#include "spdlog/cfg/env.h"

Logger::Logger() {
    spdlog::cfg::load_env_levels();
    _logger =  spdlog::stdout_color_mt("cefbrowser");
    // _logger->set_level(spdlog::level::trace);
    _logger->set_level(spdlog::level::debug);
}

Logger::~Logger() {
    shutdown();
    delete logger;
    logger = nullptr;
}

void Logger::switchToFileLogger(std::string filename) {
    size_t max_size = 1048576 * 5;
    size_t max_files = 3;
    _logger = spdlog::rotating_logger_mt("browser", filename, max_size, max_files);
    spdlog::flush_every(std::chrono::seconds(1));
    _switchedToFile = true;
}

Logger* logger = new Logger();