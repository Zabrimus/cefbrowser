#include "xhrinterception.h"
#include "json.hpp"
#include "logger.h"

void XhrInterception::modifyResponse() {
    nlohmann::json dataJson;

    if (cprResponse.status_code < 300) {
        DEBUG("[XhrInterception] Request Completed {} in {} ms", cprResponse.url.str(), cprResponse.elapsed);

        // hbbtv.zdf.de
        if (logger->isTraceEnabled()) {
            TRACE("[XhrInterception] Downloaded data:\n{}\n", cprResponse.text);
        }

        try {
            dataJson = nlohmann::ordered_json::parse(cprResponse.text);
        } catch (nlohmann::json::parse_error &e) {
            INFO("[XhrInterception] Json parse error: {}", e.what());
            return;
        }

        auto j_flat = dataJson.flatten();
        if (logger->isTraceEnabled()) {
            TRACE("[XhrInterception] Json (flatten):\n{}", to_string(j_flat));
        }

        if ((cprResponse.url.str().find("new-hbbtv.zdf.de/al/video-pages/by-canonical") != std::string::npos) && (cprResponse.url.str().find("livestreams=true") != std::string::npos)) {
            for (auto& el : j_flat.items()) {
                // std::cout << "key: " << el.key() << ", value:" << el.value() << '\n';
                if (el.key().find("h264_aac_mp4_http_mpd_http") != std::string::npos && el.key().find("url") != std::string::npos) {
                    TRACE("[XhrInterception] Found live URL: {}", el.value().get<std::string>());
                    auto url = el.value().get<std::string>();
                    auto urlEncoded = CefBase64Encode(url.c_str(), url.length());

                    j_flat[el.key()] = std::string("//localhost/encoded/") + urlEncoded.ToString() + ".mp4";
                }
            }

            j_flat.erase("/data/videoCluster/teaser/nodes");

            dataJson = j_flat.unflatten();
        }

        if (cprResponse.url.str().find("-hbbtv.zdf.de/ds/configuration") != std::string::npos) {
            if (dataJson.find("dash") != dataJson.end()) {
                dataJson["dash"] = false;
            }
        } else if (cprResponse.url.str().find("hbbtv.zdf.de") != std::string::npos) {
            // Version 1
            if (dataJson["data"] != nullptr) {
                dataJson["data"].erase("ageControl");
            }

            // Version 2
            if (dataJson["fsk"]["age"] != nullptr) {
                dataJson["fsk"].erase("age");
            }

            if (dataJson["fsk"] != nullptr) {
                dataJson.erase("fsk");
            }

            // Version 3
            if (dataJson.find("videoMetaData") != dataJson.end() && dataJson["videoMetaData"] != nullptr) {
                dataJson["videoMetaData"].erase("fsk");
            }
        } else if (cprResponse.url.str().find("tv.ardmediathek.de/dyn/get?id=video") != std::string::npos) {
            // Version 1
            if (dataJson["video"]["meta"]["maturityContentRating"] != nullptr) {
                // dataJson["video"]["meta"].erase("maturityContentRating");
                // dataJson["video"]["meta"]["maturityContentRating"]["age"] = "6";
                // dataJson["video"]["meta"]["maturityContentRating"]["isBlocked"] = false;
            }
        }
    } else {
        DEBUG("[XhrInterception] Error {}, Code {}, Text {} in {} ms", cprResponse.url.str(), cprResponse.status_code, cprResponse.status_line, cprResponse.elapsed);
        return;
    }

    cprResponse.header["Access-Control-Allow-Origin"] = "*";

    // save downloaded data
    cprResponse.text = dataJson.dump();
}