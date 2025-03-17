#pragma once

#include <string>
#include "httplib.h"

class TranscoderRemoteClient {
public:
    explicit TranscoderRemoteClient(std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort, std::string vdrIp, int vdrPort);
    ~TranscoderRemoteClient();

    std::string
    Probe(std::string url, std::string cookies, std::string referer, std::string userAgent, std::string postfix);
    bool StreamUrl(std::string url, std::string cookies, std::string referer, std::string userAgent, int32_t i);
    bool Pause();
    bool Seek(std::string seekTo);
    bool Resume(std::string position);
    bool Stop(std::string& reason) const;
    std::string GetAudioInfo();

private:
    httplib::Client *client;

    std::string transcoderIp;
    int transcoderPort;

    std::string browserIp;
    int browserPort;

    std::string vdrIp;
    int vdrPort;
};
