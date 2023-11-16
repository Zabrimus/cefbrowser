#include "transcoderremoteclient.h"
#include "logger.h"

TranscoderRemoteClient::TranscoderRemoteClient(std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort, std::string vdrIp, int vdrPort)
                                    : transcoderIp(transcoderIp), transcoderPort(transcoderPort),
                                      browserIp(browserIp), browserPort(browserPort),
                                      vdrIp(vdrIp), vdrPort(vdrPort) {
    client = new httplib::Client(transcoderIp, transcoderPort);
    client->set_read_timeout(15, 0);
}

TranscoderRemoteClient::~TranscoderRemoteClient() {
    delete client;
}

std::string TranscoderRemoteClient::Probe(std::string url, std::string cookies, std::string referer, std::string userAgent, std::string postfix) {
    httplib::Params params;
    params.emplace("url", url);
    params.emplace("cookies", cookies);
    params.emplace("referer", referer);
    params.emplace("userAgent", userAgent);
    params.emplace("responseIp", browserIp);
    params.emplace("responsePort", std::to_string(browserPort));
    params.emplace("vdrIp", vdrIp);
    params.emplace("vdrPort", std::to_string(vdrPort));

    params.emplace("postfix", postfix);

    if (auto res = client->Post("/Probe", params)) {
        if (res->status != 200) {
            TRACE("Http result: {}", res->status);
            return "";
        } else {
            TRACE("Probe sent: {}", res->status);
            return res->body;
        }
    } else {
        auto err = res.error();
        ERROR("Http error: {}", httplib::to_string(err));
        return "";
    }
}

bool TranscoderRemoteClient::StreamUrl(std::string url, std::string cookies, std::string referer, std::string userAgent) {
    httplib::Params params;
    params.emplace("url", url);
    params.emplace("cookies", cookies);
    params.emplace("referer", referer);
    params.emplace("userAgent", userAgent);
    params.emplace("responseIp", browserIp);
    params.emplace("responsePort", std::to_string(browserPort));
    params.emplace("vdrIp", vdrIp);
    params.emplace("vdrPort", std::to_string(vdrPort));

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

bool TranscoderRemoteClient::Stop(std::string& reason) const {
    httplib::Params params;
    params.emplace("streamId", browserIp + "_" + std::to_string(browserPort));
    params.emplace("reason", reason);

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
