#include "transcoderremoteclient.h"
#include "logger.h"

TranscoderRemoteClient::TranscoderRemoteClient(std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort)
                                    : transcoderIp(transcoderIp), transcoderPort(transcoderPort),  browserIp(browserIp), browserPort(browserPort) {
    client = new httplib::Client(transcoderIp, transcoderPort);
}

TranscoderRemoteClient::~TranscoderRemoteClient() {
    delete client;
}

bool TranscoderRemoteClient::StreamUrl(std::string url, std::string cookies, std::string referer, std::string userAgent) {
    httplib::Params params;
    params.emplace("url", url);
    params.emplace("cookies", cookies);
    params.emplace("referer", referer);
    params.emplace("userAgent", userAgent);
    params.emplace("responseIp", browserIp);
    params.emplace("responsePort", std::to_string(browserPort));

    if (auto res = client->Post("/StreamUrl", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return false;
        } else {
            TRACE("StreamURl sent: {}", res->status);
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return false;
    }

    return true;
}


bool TranscoderRemoteClient::Pause() {
    httplib::Params params;
    params.emplace("streamId", browserIp + "_" + std::to_string(browserPort));

    if (auto res = client->Post("/Pause", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Seek(std::string seekTo) {
    httplib::Params params;
    params.emplace("streamId", browserIp + "_" + std::to_string(browserPort));
    params.emplace("seekTo", seekTo);

    if (auto res = client->Post("/SeekTo", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Resume(std::string position) {
    httplib::Params params;
    params.emplace("streamId", browserIp + "_" + std::to_string(browserPort));
    params.emplace("position", position);

    if (auto res = client->Post("/Resume", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Stop() const {
    httplib::Params params;
    params.emplace("streamId", browserIp + "_" + std::to_string(browserPort));

    if (auto res = client->Post("/Stop", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return false;
    }

    return true;
}
