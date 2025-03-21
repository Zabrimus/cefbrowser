#include <chrono>
#include <thread>

#include "v8handler.h"
#include "logger.h"
#include "browserclient.h"

bool startVideo;
bool videoReset = false;
bool stopVideoThreadRunning = false;

std::string videoInfo;
std::string oldVideoInfo;

void V8Handler::stopVdrVideo() {
    int waitTime = 10; // ms
    int count = 5000 / waitTime;
    startVideo = false;

    while (count-- > 0 && !startVideo) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }

    if (startVideo) {
        TRACE("Thread stopVdrVideo, resetVideo");

        vdrRemoteClient->ResetVideo(videoInfo);
        videoReset = true;
    } else {
        TRACE("Thread stopVdrVideo, stopVideo");

        videoInfo = "";
        vdrRemoteClient->StopVideo();
        videoReset = false;
    }

    stopVideoThreadRunning = false;
}

V8Handler::V8Handler(BrowserParameter bParam) : bParam(bParam)
{
    transcoderRemoteClient = new TranscoderRemoteClient(bParam.transcoderIp, bParam.transcoderPort, bParam.browserIp, bParam.browserPort, bParam.vdrIp, bParam.vdrPort);
    vdrRemoteClient = new VdrRemoteClient(bParam.vdrIp, bParam.vdrPort);

    lastVideoX = lastVideoY = lastVideoW = lastVideoH = 0;
    lastFullscreen = false;

    videoInfo = "";
    videoReset = false;
    stopVideoThreadRunning = false;
}

V8Handler::~V8Handler() {
    delete transcoderRemoteClient;
    delete vdrRemoteClient;
}

void V8Handler::sendMessageToProcess(std::string message) {
    sendMessageToProcess(message, "");
}

void V8Handler::sendMessageToProcess(std::string message, std::string parameter) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    if (!parameter.empty()) {
        args->SetString(0, parameter);
    }

    browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
}

void V8Handler::sendMessageToProcess(std::string message, std::vector<std::string>& parameter) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();

    int idx = 0;
    for (const auto &item: parameter) {
        args->SetString(idx, item);
        idx++;
    }

    browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
}

bool V8Handler::Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception) {
    DEBUG("V8Handler::Execute: {}", name.ToString());
    time_t now = time(nullptr);

    if (name == "StreamVideo") {
        if (!arguments.empty()) {
            const auto& urlParam = arguments.at(0);
            auto url = urlParam.get()->GetStringValue();

            const auto& cookiesParam = arguments.at(1);
            auto cookies = cookiesParam.get()->GetStringValue();

            const auto& refererParam = arguments.at(2);
            auto referer = refererParam.get()->GetStringValue();

            const auto& userAgentParam = arguments.at(3);
            auto userAgent = userAgentParam.get()->GetStringValue();

            const auto& mpdStartParam = arguments.at(4);
            auto mpdStart = mpdStartParam.get()->GetIntValue();

            TRACE("Video URL: {}", url.ToString());

            // 1. Step call Probe
            oldVideoInfo = videoInfo;
            videoInfo = transcoderRemoteClient->Probe(url, cookies, referer, userAgent, std::to_string(now));

            if (videoInfo.empty()) {
                ERROR("Probe failed: Wrong video URL, a server error or another error.");
                return false;
            }

            TRACE("Video Info:  {} -> {}", oldVideoInfo, videoInfo);
            if (!oldVideoInfo.empty() && oldVideoInfo != videoInfo) {
                INFO("--> Video format change");
            } else {
                INFO("--> Video format equal");
            }

            startVideo = true;

            if (videoInfo.empty()) {
                // transcoder not available
                ERROR("Probe failed: Transcoder not available, wrong video URL or another error.");
                return false;
            }

            // 2. Step call StreamUrl
            if (!transcoderRemoteClient->StreamUrl(url, cookies, referer, userAgent, mpdStart)) {
                // transcoder not available
                ERROR("Unable to send request to transcoder");
                return false;
            }

            while (stopVideoThreadRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            TRACE("VideoReset: {}", videoReset);

            if (!videoReset) {
                vdrRemoteClient->StartVideo(videoInfo);
            }
        }

        std::string newVideoUrl = "http://"+ bParam.transcoderIp + ":" + std::to_string(bParam.transcoderPort) + "/movie/transparent-video-" + bParam.browserIp + "_" + std::to_string(bParam.browserPort) + "-" + std::to_string(now) + ".webm";
        TRACE("Javascript return new Video URL: {}", newVideoUrl);

        retval = CefV8Value::CreateString(newVideoUrl);
        return true;
    } else if (name == "StopVideo") {
        std::string reason = "Javascript StopVideo";
        transcoderRemoteClient->Stop(reason);

        stopVideoThreadRunning = true;
        std::thread stopThread(&V8Handler::stopVdrVideo, this);
        stopThread.detach();

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
            vdrRemoteClient->ResetVideo(videoInfo);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "RedButton") {
        if (!arguments.empty()) {
            const auto& param = arguments.at(0);
            auto channelId = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute RedButton Argument ChannelId {}", channelId);

            sendMessageToProcess("RedButton", channelId);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "LoadUrl") {
        if (!arguments.empty()) {
            const auto& param = arguments.at(0);
            auto url = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute LoadUrl {}", url);

            sendMessageToProcess("LoadUrl", url);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    }  else if (name == "VideoSize") {
        if (!arguments.empty()) {
            const auto x = arguments.at(0)->GetIntValue();
            const auto y = arguments.at(1)->GetIntValue();
            const auto w = arguments.at(2)->GetIntValue();
            const auto h = arguments.at(3)->GetIntValue();

            // TODO: Ist das noch notwendig? Aufgrund der Änderungen auf mutation-summary sollte das nicht mehr passieren.
            if ((w <= 160 || h <= 100) || (w == 300 && h == 150 && x == 0)) {
                // ignore these video sizes
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
                sendMessageToProcess("SetDirtyOSD");
            }
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

            sendMessageToProcess("StartApp", params);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "AudioInfo") {
        std::string result = transcoderRemoteClient->GetAudioInfo();
        retval = CefV8Value::CreateString(result);
        return true;
    } else if (name == "SelectAudioTrack") {
        if (!arguments.empty()) {
            const auto nr = arguments.at(0)->GetIntValue();
            bool result = vdrRemoteClient->SelectAudioTrack(nr);
            retval = CefV8Value::CreateBool(result);
            return true;
        }
        retval = CefV8Value::CreateBool(true);
        return true;
    } else {
        ERROR("Javascript function {} is not implemented.", name.ToString());
    }

    // Function does not exist.
    return false;
}
