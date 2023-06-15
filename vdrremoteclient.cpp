#include "vdrremoteclient.h"
#include "logger.h"

std::mutex httpMutex;

VdrRemoteClient::VdrRemoteClient(std::string vdrIp, int vdrPort) {
    client = new httplib::Client(vdrIp, vdrPort);
}

VdrRemoteClient::~VdrRemoteClient() {
    delete client;
}

bool VdrRemoteClient::ProcessOsdUpdate(int width, int height) {
    const std::lock_guard<std::mutex> lock(httpMutex);

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

bool VdrRemoteClient::ProcessTSPacket(std::string packets) const {
    const std::lock_guard<std::mutex> lock(httpMutex);

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
    const std::lock_guard<std::mutex> lock(httpMutex);

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
    const std::lock_guard<std::mutex> lock(httpMutex);

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

bool VdrRemoteClient::Pause() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    if (auto res = client->Get("/PauseVideo")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (PauseVideo): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::Resume() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    if (auto res = client->Get("/ResumeVideo")) {
        if (res->status != 200) {
            std::cout << "Http result: " << res->status << std::endl;
            return false;
        }
    } else {
        auto err = res.error();
        std::cout << "HTTP error (ResumeVideo): " << httplib::to_string(err) << std::endl;
        return false;
    }

    return true;
}

bool VdrRemoteClient::VideoSize(int x, int y, int w, int h) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    httplib::Params params;
    params.emplace("x", std::to_string(x));
    params.emplace("y", std::to_string(y));
    params.emplace("w", std::to_string(w));
    params.emplace("h", std::to_string(h));

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
    const std::lock_guard<std::mutex> lock(httpMutex);

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
