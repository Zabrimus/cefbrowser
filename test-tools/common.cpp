#include <iostream>
#include <getopt.h>
#include "mini/ini.h"
#include "logger.h"
#include "common.h"

std::string transcoderIp;
int transcoderPort;

std::string browserIp;
int browserPort;

std::string vdrIp;
int vdrPort;

std::string videoFile;

bool readConfiguration(std::string configFile) {
    mINI::INIFile file(configFile.c_str());
    mINI::INIStructure ini;
    auto result = file.read(ini);

    if (!result) {
        ERROR("Unable to read config file: {}", configFile);
        return false;
    }

    try {
        browserIp = ini["browser"]["http_ip"];
        if (browserIp.empty()) {
            ERROR("http ip (browser) not found");
            return false;
        }

        std::string tmpBrowserPort = ini["browser"]["http_port"];
        if (tmpBrowserPort.empty()) {
            ERROR("http port (browser) not found");
            return false;
        }

        transcoderIp = ini["transcoder"]["http_ip"];
        if (transcoderIp.empty()) {
            ERROR("http ip (transcoder) not found");
            return false;
        }

        std::string tmpTranscoderPort = ini["transcoder"]["http_port"];
        if (tmpTranscoderPort.empty()) {
            ERROR("http port (transcoder) not found");
            return false;
        }

        vdrIp = ini["vdr"]["http_ip"];
        if (vdrIp.empty()) {
            ERROR("http ip (vdr) not found");
            return false;
        }

        std::string tmpVdrPort = ini["vdr"]["http_port"];
        if (tmpVdrPort.empty()) {
            ERROR("http port (vdr) not found");
            return false;
        }

        browserPort = std::stoi(tmpBrowserPort);
        transcoderPort = std::stoi(tmpTranscoderPort);
        vdrPort = std::stoi(tmpVdrPort);
    } catch (...) {
        ERROR("configuration error. aborting...");
        return false;
    }

    return true;
}

bool parseConfiguration(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " -c <config_file>" << std::endl;
        return false;
    }

    static struct option long_options[] = {
            {"config", required_argument, nullptr, 'c'},
            {"video", optional_argument, nullptr, 'v'},
            {nullptr}
    };

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "c:v:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                if (!readConfiguration(optarg)) {
                    exit(-1);
                }
                break;

            case 'v':
                videoFile = optarg;
                break;
        }
    }

    return true;
}
