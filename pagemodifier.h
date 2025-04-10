#pragma once

#include "logger.h"
#include "cef_includes.h"
#include <string>
#include "json.hpp"
#include "tools.h"
#include <utility>
#include <map>
#include <cstdio>

class PageModifier {
public:
    PageModifier() = default;

    void init(std::string sp);

    std::string injectAll(std::string& source);

private:
    inline std::string getDynamicJs();
    inline std::string getDynamicZoom();

    static std::string insertAfterHead(std::string& origStr, std::string& addStr);
    static std::string insertBeforeEnd(std::string& origStr, std::string& addStr);

private:
    std::string staticPath;
    std::map<std::string, std::string> cache;
};
