#include <chrono>
#include <thread>

#include "v8handler.h"
#include "logger.h"

bool startVideo;
bool videoReset;

void V8Handler::stopVdrVideo() {
    int waitTime = 5; // ms
    int count = 500 / waitTime;
    startVideo = false;

    while (count-- > 0 && !startVideo) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }

    if (startVideo) {
        vdrRemoteClient->ResetVideo();
        videoReset = true;
    } else {
        vdrRemoteClient->StopVideo();
        videoReset = false;
    }
}

V8Handler::V8Handler(std::string bIp, int bPort, std::string tIp, int tPort, std::string vdrIp, int vdrPort)
        : browserIp(bIp), browserPort(bPort), transcoderIp(tIp), transcoderPort(tPort), vdrIp(vdrIp), vdrPort(vdrPort)
{
    transcoderRemoteClient = new TranscoderRemoteClient(tIp, tPort, bIp, bPort);
    vdrRemoteClient = new VdrRemoteClient(vdrIp, vdrPort);

    lastVideoX = lastVideoY = lastVideoW = lastVideoH = 0;
    lastFullscreen = false;
}

V8Handler::~V8Handler() {
    delete transcoderRemoteClient;
    delete vdrRemoteClient;
}

void V8Handler::sendMessageToProcess(std::string message, CefProcessId target_process) {
    sendMessageToProcess(message, "", target_process);
}

void V8Handler::sendMessageToProcess(std::string message, std::string parameter, CefProcessId target_process) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    if (!parameter.empty()) {
        args->SetString(0, parameter);
    }

    browser->GetMainFrame()->SendProcessMessage(target_process, msg);
}

void V8Handler::sendMessageToProcess(std::string message, std::vector<std::string>& parameter, CefProcessId target_process) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();

    int idx = 0;
    for (const auto &item: parameter) {
        args->SetString(idx, item);
        idx++;
    }

    browser->GetMainFrame()->SendProcessMessage(target_process, msg);
}

bool V8Handler::Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception) {
    DEBUG("V8Handler::Execute: {}", name.ToString());

    if (name == "StreamVideo") {
        if (!arguments.empty()) {
            startVideo = true;

            const auto& urlParam = arguments.at(0);
            auto url = urlParam.get()->GetStringValue();

            if (!transcoderRemoteClient->StreamUrl(url)) {
                // transcoder not available
                ERROR("Unable to send request to transcoder");
                return false;
            }

            if (!videoReset) {
                vdrRemoteClient->StartVideo();
            }
        }

        retval = CefV8Value::CreateString("http://"+ transcoderIp + ":" + std::to_string(transcoderPort) + "/movie/transparent-video-" + browserIp + "_" + std::to_string(browserPort) + ".webm");
        return true;
    } else if (name == "StopVideo") {
        transcoderRemoteClient->Stop();

        std::thread stopThread(&V8Handler::stopVdrVideo, this);
        stopThread.detach();

        /*
        vdrRemoteClient->StopVideo();
        // TEST: wait some seconds to give transcoder and VDR to change to stop everything
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 4));
        */

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "PauseVideo") {
        vdrRemoteClient->Pause();
        transcoderRemoteClient->Pause();

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "ResumeVideo") {
        const auto& param = arguments.at(0);
        auto position = param.get()->GetStringValue().ToString();

        DEBUG("RESUME at timestamp {}", position);

        vdrRemoteClient->Resume();
        transcoderRemoteClient->Resume(position);

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "SeekVideo") {
        if (!arguments.empty()) {
            const auto& param = arguments.at(0);
            auto position = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute SeekVideo Argument Pos {}", position);

            transcoderRemoteClient->Seek(position);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "RedButton") {
        if (!arguments.empty()) {
            const auto& param = arguments.at(0);
            auto channelId = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute RedButton Argument ChannelId {}", channelId);

            sendMessageToProcess("RedButton", channelId, PID_BROWSER);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "LoadUrl") {
        if (!arguments.empty()) {
            const auto& param = arguments.at(0);
            auto url = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute LoadUrl {}", url);

            sendMessageToProcess("LoadUrl", url, PID_BROWSER);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    }  else if (name == "VideoSize") {
        if (!arguments.empty()) {
            const auto x = arguments.at(0)->GetIntValue();
            const auto y = arguments.at(1)->GetIntValue();
            const auto w = arguments.at(2)->GetIntValue();
            const auto h = arguments.at(3)->GetIntValue();

            // sanity check. A too small video size will be ignored
            if (w <= 160 || h <= 100) {
                retval = CefV8Value::CreateBool(true);
                return true;
            }

            if ((x != lastVideoX) || (y != lastVideoY) || (w != lastVideoW) || (h != lastVideoH)) {
                // send new VideoSize to Browser
                lastVideoX = x;
                lastVideoY = y;
                lastVideoH = h;
                lastVideoW = w;
                lastFullscreen = false;

                vdrRemoteClient->VideoSize(x, y, w, h);
            }

            sendMessageToProcess("SetDirtyOSD", PID_BROWSER);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "VideoFullscreen") {
        if (!lastFullscreen) {
            lastVideoX = 0;
            lastVideoY = 0;
            lastVideoH = 0;
            lastVideoW = 0;
            lastFullscreen = true;

            vdrRemoteClient->VideoFullscreen();
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "StartApp") {
        if (!arguments.empty()) {
            const auto channelId = arguments.at(0)->GetStringValue();
            const auto appId = arguments.at(1)->GetStringValue();
            const auto args = arguments.at(2)->GetStringValue();

            std::vector<std::string> params;
            params.push_back(channelId);
            params.push_back(appId);
            params.push_back(args);

            sendMessageToProcess("StartApp", params, PID_BROWSER);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else {
        ERROR("Javascript function {} is not implemented.", name.ToString());
    }

    // Function does not exist.
    return false;
}
