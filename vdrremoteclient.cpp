#include "vdrremoteclient.h"
#include "logger.h"

VdrRemoteClient* vdrRemoteClient;

VdrRemoteClient::VdrRemoteClient(std::string vdrIp, int vdrPort) {
    client = new httplib::Client(vdrIp, vdrPort);
    vdrRemoteClient = this;
}

VdrRemoteClient::~VdrRemoteClient() {
    delete client;
    vdrRemoteClient = nullptr;
}

bool VdrRemoteClient::ProcessOsdUpdate(int width, int height) {
    httplib::Params params;
    params.emplace("width", std::to_string(width));
    params.emplace("height", std::to_string(height));

    if (auto res = client->Post("/ProcessOsdUpdate", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (ProcessOsdUpdate): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::ProcessTSPacket(std::string packets) {
    if (auto res = client->Post("/ProcessTSPacket", packets, "text/plain")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (ProcessTSPacket): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::StartVideo() {
    if (auto res = client->Get("/StartVideo")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (StartVideo): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::StopVideo() {
    if (auto res = client->Get("/StopVideo")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (StopVideo): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::VideoSize(std::string x, std::string y, std::string w, std::string h) {
    httplib::Params params;
    params.emplace("x", x);
    params.emplace("y", y);
    params.emplace("w", w);
    params.emplace("h", h);

    if (auto res = client->Post("/VideoSize", params)) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (VideoSize): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::VideoFullscreen() {
    if (auto res = client->Get("/VideoFullscreen")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (VideoFullscreen): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}
