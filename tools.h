#pragma once

#include <string>

enum image_type_enum {
    NONE = 0,
    QOI = 1
};

typedef struct BrowserParameter {
    std::string vdrIp;
    int vdrPort;

    std::string transcoderIp;
    int transcoderPort;

    std::string browserIp;
    int browserPort;

    image_type_enum osdqoi;

    int zoom_width;
    int zoom_height;

    bool use_dirty_recs;

    std::string static_path;
    std::string user_agent_path;

    bool bindAll;

    image_type_enum image_type;
} BrowserParameter;

bool endsWith(const std::string& str, const std::string& suffix);
bool startsWith(const std::string& str, const std::string& prefix);
std::string readFile(const char *filename);
