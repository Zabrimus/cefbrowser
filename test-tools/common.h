#pragma once

#include <string>

extern std::string transcoderIp;
extern int transcoderPort;

extern std::string browserIp;
extern int browserPort;

extern std::string vdrIp;
extern int vdrPort;

extern std::string videoFile;

bool parseConfiguration(int argc, char* argv[]);