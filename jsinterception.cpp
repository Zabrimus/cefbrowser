#include "jsinterception.h"
#include "json.hpp"
#include "logger.h"

void JSInterception::modifyResponse() {
    if (cprResponse.status_code < 300) {
        DEBUG("[JSInterception] Request Completed {} in {} ms", cprResponse.url.str(), cprResponse.elapsed);
        if (cprResponse.url.str().find("new-hbbtv.zdf.de/static/js/main.") != std::string::npos) {
            // always use HTML5 video object
            // Alt: window.location.pathname.toLowerCase().includes("dash.html")
            // Neu: true
            std::string searchStrHtml5  = "window.location.pathname.toLowerCase().includes(\"dash.html\")";
            std::string replaceStrHtml5 = "true                                                        ";

            auto pos = cprResponse.text.find(searchStrHtml5);
            if (pos != std::string::npos) {
                cprResponse.text = std::string(cprResponse.text.substr(0, pos))
                                + replaceStrHtml5
                                + std::string(cprResponse.text.substr(pos + searchStrHtml5.length()));
            }

            // set default autoplay = false:
            // Alt: =n.autoplay
            // Neu: =false
            std::string searchStrAutoplay  = "=n.autoplay";
            std::string replaceStrAutoplay = "=false     ";

            auto posAutoplay = cprResponse.text.find(searchStrAutoplay);
            if (posAutoplay != std::string::npos) {
                cprResponse.text = std::string(cprResponse.text.substr(0, posAutoplay))
                                + replaceStrAutoplay
                                + std::string(cprResponse.text.substr(posAutoplay + searchStrAutoplay.length()));
            }
        }
    }
}