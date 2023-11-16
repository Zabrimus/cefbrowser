#include "vdrremoteclient.h"
#include "logger.h"

std::mutex httpMutex;

VdrRemoteClient::VdrRemoteClient(std::string vdrIp, int vdrPort) {
    TRACE("Create new VdrRemoteClient");

    client = new httplib::Client(vdrIp, vdrPort);
    client->set_read_timeout(15, 0);

    // Send Hello
    SendHello();
}

VdrRemoteClient::~VdrRemoteClient() {
    TRACE("Destruct VdrRemoteClient");

    delete client;
}

bool VdrRemoteClient::ProcessOsdUpdate(int disp_width, int disp_height, int x, int y, int width, int height) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    // TRACE("Call VdrRemoteClient::ProcessOsdUpdate");

    httplib::Params params;
    params.emplace("disp_width", std::to_string(disp_width));
    params.emplace("disp_height", std::to_string(disp_height));
    params.emplace("x", std::to_string(x));
    params.emplace("y", std::to_string(y));
    params.emplace("width", std::to_string(width));
    params.emplace("height", std::to_string(height));

    if (auto res = client->Post("/ProcessOsdUpdate", params)) {
        if (res->status != 200) {
            // ERROR("Http result: {}", res->status);
            return false;
        } else {
            // TRACE("Http result: {}", res->status);
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (ProcessOsdUpdate): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::ProcessOsdUpdateQoi(int disp_width, int disp_height, int x, int y, const std::string& imageQoi) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::ProcessOsdUpdateQoi");

    if (auto res = client->Post("/ProcessOsdUpdateQOI", std::to_string(disp_width) + ":" + std::to_string(disp_height) + ":" + std::to_string(x) + ":" + std::to_string(y) + ":" + imageQoi, "text/plain")) {
        if (res->status != 200) {
            // ERROR("Http result: {}", res->status);
            return false;
        } else {
            // TRACE("Http result: {}", res->status);
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (ProcessOsdUpdate): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::ProcessTSPacket(std::string packets) const {
    const std::lock_guard<std::mutex> lock(httpMutex);

    // TRACE("Call VdrRemoteClient::ProcessTSPacket");

    if (auto res = client->Post("/ProcessTSPacket", packets, "text/plain")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (ProcessTSPacket): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::StartVideo(std::string videoInfo) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::StartVideo");

    httplib::Params params;
    params.emplace("videoInfo", videoInfo);

    if (auto res = client->Post("/StartVideo", params)) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (StartVideo): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::StopVideo() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::StopVideo");

    if (auto res = client->Get("/StopVideo")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (StopVideo): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::Pause() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::Pause");

    if (auto res = client->Get("/PauseVideo")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (PauseVideo): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::Resume() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::Resume");

    if (auto res = client->Get("/ResumeVideo")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (ResumeVideo): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::VideoSize(int x, int y, int w, int h) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::VideoSize");

    httplib::Params params;
    params.emplace("x", std::to_string(x));
    params.emplace("y", std::to_string(y));
    params.emplace("w", std::to_string(w));
    params.emplace("h", std::to_string(h));

    if (auto res = client->Post("/VideoSize", params)) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (VideoSize): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::VideoFullscreen() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::VideoFullscreen");

    if (auto res = client->Get("/VideoFullscreen")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (VideoFullscreen): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::SendHello() {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::SendHello");

    if (auto res = client->Get("/Hello")) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (SendHello): {}", httplib::to_string(err));
        return false;
    }

    return true;
}

bool VdrRemoteClient::ResetVideo(std::string videoInfo) {
    const std::lock_guard<std::mutex> lock(httpMutex);

    TRACE("Call VdrRemoteClient::ResetVideo");

    httplib::Params params;
    params.emplace("videoInfo", videoInfo);

    if (auto res = client->Post("/ResetVideo", params)) {
        if (res->status != 200) {
            ERROR("Http result: {}", res->status);
            return false;
        }
    } else {
        auto err = res.error();
        ERROR("HTTP error (ResetVideo): {}", httplib::to_string(err));
        return false;
    }

    return true;
}