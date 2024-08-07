#pragma once

#include <string>
#include "httplib.h"

class VdrRemoteClient {
public:
    explicit VdrRemoteClient(std::string vdrIp, int vdrPort);
    ~VdrRemoteClient();

    bool ProcessOsdUpdate(int disp_width, int disp_height, int x, int y, int width, int height);
    bool ProcessOsdUpdateQoi(int disp_width, int disp_height, int x, int y, const std::string& imageQoi);
    bool ProcessTSPacket(std::string packets) const;

    bool StartVideo(std::string videoInfo);
    bool StopVideo();
    bool Pause();
    bool Resume();
    bool ResetVideo(std::string videoInfo);

    bool VideoSize(int x, int y, int w, int h);
    bool VideoFullscreen();

    bool SelectAudioTrack(int nr);

    bool SendHello();
private:
    httplib::Client* client;
};

