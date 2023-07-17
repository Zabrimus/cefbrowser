#pragma once

#include <string>
#include "httplib.h"

class VdrRemoteClient {
public:
    explicit VdrRemoteClient(std::string vdrIp, int vdrPort);
    ~VdrRemoteClient();

    bool ProcessOsdUpdate(int width, int height);
    bool ProcessOsdUpdateQoi(const std::string& imageQoi);
    bool ProcessTSPacket(std::string packets) const;

    bool StartVideo();
    bool StopVideo();
    bool Pause();
    bool Resume();
    bool ResetVideo();

    bool VideoSize(int x, int y, int w, int h);
    bool VideoFullscreen();

    bool SendHello();

private:
    httplib::Client* client;
};

