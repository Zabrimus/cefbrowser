#include "logger.h"
#include "spdlog/cfg/env.h"

Logger::Logger() {
    spdlog::cfg::load_env_levels();

    _logger =  spdlog::stdout_color_mt("cefbrowser");
    _logger->set_level(spdlog::level::trace);

    logger = this;
}

Logger::~Logger() {
    shutdown();
    delete logger;
    logger = nullptr;
}

Logger* logger = new Logger();