#pragma once

#include <string>

bool endsWith(const std::string& str, const std::string& suffix);
bool startsWith(const std::string& str, const std::string& prefix);
std::string readFile(const char *filename);
