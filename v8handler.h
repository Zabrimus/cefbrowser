#pragma once

#include "cef_includes.h"
#include "transcoderremoteclient.h"
#include "vdrremoteclient.h"

class V8Handler : public CefV8Handler {
public:
    V8Handler(std::string bIp, int bPort, std::string tIp, int tPort, std::string vdrIp, int vdrPort);
    ~V8Handler();

    bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) override;

private:
    std::string browserIp;
    int browserPort;

    std::string transcoderIp;
    int transcoderPort;

    std::string vdrIp;
    int vdrPort;

    VdrRemoteClient* vdrRemoteClient;
    TranscoderRemoteClient* transcoderRemoteClient;

private:
    void sendMessageToBrowser(std::string message);
    void sendMessageToBrowser(std::string message, std::string parameter);
    void sendMessageToBrowser(std::string message, std::vector<std::string>& parameter);

    void stopVdrVideo();

    IMPLEMENT_REFCOUNTING(V8Handler);
};
