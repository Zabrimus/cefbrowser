#pragma once

#include <string>
#include "httplib.h"

class VdrRemoteClient {
public:
    explicit VdrRemoteClient(std::string vdrIp, int vdrPort);
    ~VdrRemoteClient();

    bool ProcessOsdUpdate(int width, int height);
    bool ProcessTSPacket(std::string packets) const;

    bool StartVideo();
    bool StopVideo();
    bool Pause();
    bool Resume();

    bool VideoSize(int x, int y, int w, int h);
    bool VideoFullscreen();

    bool SendHello();

private:
    httplib::Client* client;
};

