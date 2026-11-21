#include <fstream>
#include "tools.h"

BrowserOptions browserOptions;

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
