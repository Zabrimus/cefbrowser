#pragma once

#include <string>
#include <thread>
#include "process.hpp"
#include "vdrremoteclient.h"

class FFmpegHandler {
private:
    std::string ffmpeg;

    TinyProcessLib::Process *streamHandler;
    std::thread *readerThread;

public:
    explicit FFmpegHandler(VdrRemoteClient* client);
    ~FFmpegHandler();

    bool streamVideo(std::string url);
    void stopVideo();
    bool isRunning() { return readerRunning; };

private:
    int fifo;
    bool readerRunning;
    VdrRemoteClient* client;
};
