#pragma once

#include <string>
#include "httplib.h"

class VdrRemoteClient {
public:
    explicit VdrRemoteClient(std::string vdrIp, int vdrPort);
    ~VdrRemoteClient();

    bool ProcessOsdUpdate(int width, int height);
    bool ProcessTSPacket(std::string packets);

    bool StartVideo();
    bool StopVideo();

    bool VideoSize(std::string x, std::string y, std::string w, std::string h);
    bool VideoFullscreen();

private:
    httplib::Client* client;
};

extern VdrRemoteClient* vdrRemoteClient;
