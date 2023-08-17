#pragma once

#include <string>
#include "httplib.h"

class TranscoderRemoteClient {
public:
    explicit TranscoderRemoteClient(std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort);
    ~TranscoderRemoteClient();

    bool StreamUrl(std::string url, std::string cookies);
    bool Pause();
    bool Seek(std::string seekTo);
    bool Resume(std::string position);
    bool Stop() const;

private:
    httplib::Client *client;

    std::string transcoderIp;
    int transcoderPort;

    std::string browserIp;
    int browserPort;
};
