#pragma once

#include <string>
#include <atomic>

const std::string BLANK_PAGE = "http://localhost/internal/blank/page";

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

    // Graphics plane, in pixels, and the ratio between it and the 1280x720
    // co-ordinate system HbbTV applications are written against.
    int zoom_width;
    int zoom_height;
    double zoom_level;

    bool use_dirty_recs;

    std::string static_path;
    std::string user_agent_path;

    bool bindAll;
    bool measure = false;

    image_type_enum image_type;
} BrowserParameter;

// Set from --ignore-certificate-errors. The browser fetches pages itself
// through libcurl, which never sees Chromium's command line, so without this
// the two disagree about which certificates are acceptable.
extern bool ignoreCertificateErrors;

bool endsWith(const std::string& str, const std::string& suffix);

// The https form of an http URL when the host serves it, else the URL as given.
std::string preferHttps(const std::string &url);

// height and zoom level for a page width (1280, 1920, 2560, 3840). Returns false for other widths.
bool zoomForWidth(int width, int &height, double &zoomLevel);

// writes the script which sets the zoom of every page
void writeZoomScript(const std::string& staticPath, int width, int height, double zoomLevel);
bool startsWith(const std::string& str, const std::string& prefix);
std::string readFile(const char *filename);

// options of the VDR setup, used by the next page
struct BrowserOptions {
    std::atomic<bool> mse { true };          // Media Source Extensions for VDR
    std::atomic<bool> zdfQuirks { true };    // special handling of the ZDF applications
    std::atomic<bool> sharedMemory { false };  // the OSD in shared memory, VDR on the same machine
};

extern BrowserOptions browserOptions;
