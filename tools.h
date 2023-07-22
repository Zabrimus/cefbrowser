#pragma once

#include <string>

enum image_type_enum {
    NONE = 0,
    QOI = 1,
    QOIR = 2
};

bool endsWith(const std::string& str, const std::string& suffix);
bool startsWith(const std::string& str, const std::string& prefix);
std::string readFile(const char *filename);
