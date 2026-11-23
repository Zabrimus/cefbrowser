#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include "tools.h"
#include "logger.h"
#include "cpr/api.h"
#include "cpr/ssl_options.h"
#include "cpr/timeout.h"

BrowserOptions browserOptions;

// Chromium force upgrades requests to hosts on its HSTS preload list, but the
// document itself comes out of the interceptor and keeps the scheme it was
// asked for. The page then sits on http while everything it asks for is
// fetched over https - a different origin - and CORS drops the application's
// own scripts, fonts and data. Load the document over https wherever the
// server answers there, so both end up on one origin.
//
// Applications navigate between their own pages often, so the answer is kept
// per host: one probe per host, not one per page. A probe that timed out or
// never connected is not kept, the next page asks again.
std::string preferHttps(const std::string &url) {
    if (url.rfind("http://", 0) != 0) {
        return url;
    }

    // Never for the browser's own static content, which is http only.
    if (url.rfind(BLANK_PAGE, 0) == 0 || url.find("://localhost") != std::string::npos) {
        return url;
    }

    const std::string rest = url.substr(strlen("http://"));
    const std::string host = rest.substr(0, rest.find_first_of("/?#"));
    const std::string httpsUrl = "https://" + rest;

    static std::mutex cacheMutex;
    static std::map<std::string, bool> httpsHosts;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = httpsHosts.find(host);
        if (it != httpsHosts.end()) {
            return it->second ? httpsUrl : url;
        }
    }

    // Same certificate policy as the rest of the browser, so this does not
    // reject a host Chromium would have used over https anyway.
    const cpr::VerifySsl verify{!ignoreCertificateErrors};
    cpr::Response probe = cpr::Head(cpr::Url{httpsUrl}, cpr::Timeout{3000}, verify);

    // A server that answers GET but not HEAD says nothing either way about
    // whether the page is there.
    if (probe.status_code == 405 || probe.status_code == 501) {
        probe = cpr::Get(cpr::Url{httpsUrl}, cpr::Timeout{3000}, verify);
    }

    const cpr::ErrorCode error = probe.error.code;
    const bool https = error == cpr::ErrorCode::OK && probe.status_code >= 200 && probe.status_code < 400;
    // Keep what the host said, or a closed port or a failed handshake - not a
    // timeout or a name that did not resolve, which can be gone the next time.
    const bool definite = error == cpr::ErrorCode::OK || error == cpr::ErrorCode::COULDNT_CONNECT
                          || error == cpr::ErrorCode::SSL_CONNECT_ERROR
                          || error == cpr::ErrorCode::PEER_FAILED_VERIFICATION;
    if (definite) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        httpsHosts[host] = https;
    }

    if (https) {
        INFO("Loading {} over https", host);
        return httpsUrl;
    }

    DEBUG("No https for {}, keeping http", url);
    return url;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::string readFile(const char *filename)
{
    std::FILE *fp = std::fopen(filename, "rb");
    if (fp) {
        std::string contents;
        std::fseek(fp, 0, SEEK_END);
        contents.resize(std::ftell(fp));
        std::rewind(fp);
        std::fread(&contents[0], 1, contents.size(), fp);
        std::fclose(fp);
        return(contents);
    }
    return "";
}

bool zoomForWidth(int width, int &height, double &zoomLevel) {
    switch (width) {
        case 1280: height = 720;  zoomLevel = 1.0; return true;
        case 1920: height = 1080; zoomLevel = 1.5; return true;
        case 2560: height = 1440; zoomLevel = 2.0; return true;
        case 3840: height = 2160; zoomLevel = 3.0; return true;
        default:   return false;
    }
}

void writeZoomScript(const std::string& staticPath, int width, int height, double zoomLevel) {
    // Scaling to the graphics plane is the screen's device scale factor
    // (BrowserClient::GetScreenInfo), so nothing here zooms: a CSS zoom on top
    // of a body sized to the plane laid the page out zoomLevel times larger than
    // the plane. The body still gets the co-ordinate system explicitly,
    // applications are written against a full screen body and one left at its
    // intrinsic size collapses to nothing.
    std::ofstream zoom;
    zoom.open(staticPath + "/js/_zoom_level.js", std::ios_base::trunc);
    zoom << "document.body.style[\"width\"] = \"" << (int)(width / zoomLevel) << "px\";\n" << std::endl;
    zoom << "document.body.style[\"height\"] = \"" << (int)(height / zoomLevel) << "px\";\n" << std::endl;
    zoom.close();
}
