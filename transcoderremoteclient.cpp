#include "transcoderremoteclient.h"

TranscoderRemoteClient* transcoderRemoteClient;

TranscoderRemoteClient::TranscoderRemoteClient(std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort)
                                    : transcoderIp(transcoderIp), transcoderPort(transcoderPort),  browserIp(browserIp), browserPort(browserPort) {
    client = new httplib::Client(transcoderIp, transcoderPort);
    transcoderRemoteClient = this;
}

TranscoderRemoteClient::~TranscoderRemoteClient() {
    delete client;
    transcoderRemoteClient = nullptr;
}

bool TranscoderRemoteClient::StreamUrl(std::string url) {
    httplib::Params params;
    params.emplace("url", url);
    params.emplace("responseIp", browserIp);
    params.emplace("responsePort", std::to_string(browserPort));

    if (auto res = client->Post("/StreamUrl", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}


bool TranscoderRemoteClient::Pause() {
    httplib::Params params;
    params.emplace("streamId", browserIp + ":" + std::to_string(browserPort));

    if (auto res = client->Post("/Pause", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Seek(std::string seekTo) {
    httplib::Params params;
    params.emplace("streamId", browserIp + ":" + std::to_string(browserPort));
    params.emplace("seekTo", seekTo);

    if (auto res = client->Post("/SeekTo", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Resume() {
    httplib::Params params;
    params.emplace("streamId", browserIp + ":" + std::to_string(browserPort));

    if (auto res = client->Post("/Resume", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool TranscoderRemoteClient::Stop() {
    httplib::Params params;
    params.emplace("streamId", browserIp + ":" + std::to_string(browserPort));

    if (auto res = client->Post("/Stop", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}
